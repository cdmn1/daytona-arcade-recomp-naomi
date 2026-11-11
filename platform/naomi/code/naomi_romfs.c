/*
 * Read-only ROMFS VFS for Naomi ROM-board / NetDIMM images.
 *
 * The on-image format is compatible with the public-domain ROMFS tooling in
 * DragonMinded/libnaomi. File contents are read on demand through
 * naomi_cart_read(); the image is never copied wholesale into Naomi RAM.
 */
#include "naomi_romfs.h"
#include "naomi_cart.h"

#include <errno.h>
#include <kos/fs.h>
#include <kos/nmmgr.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define NAOMI_HEADER_SIZE          0x500u
#define NAOMI_MAIN_SECTIONS        0x360u
#define NAOMI_TEST_SECTIONS        0x3C0u
#define NAOMI_SECTION_COUNT        8u
#define NAOMI_ROMFS_HEADER_SIZE    16u
#define NAOMI_ROMFS_ENTRY_SIZE     256u
#define NAOMI_ROMFS_TYPE_DIR       1u
#define NAOMI_ROMFS_TYPE_FILE      2u
#define NAOMI_ROMFS_CHECKSUM       0x11291985u
#define NAOMI_CART_LIMIT           0x10000000u
#define NAOMI_FILE_CACHE_SIZE      1024u

typedef struct {
    int32_t offset;
    uint32_t size;
    uint32_t type;
    char name[NAOMI_ROMFS_ENTRY_SIZE - 12u];
} naomi_romfs_entry_t;

_Static_assert(sizeof(naomi_romfs_entry_t) == NAOMI_ROMFS_ENTRY_SIZE,
               "Naomi ROMFS directory entry must be 256 bytes");

typedef struct {
    uint32_t offset;
    uint32_t size;
    uint32_t type;
} naomi_romfs_info_t;

typedef struct {
    uint32_t image_offset;
    uint32_t size;
    uint32_t position;
    uint32_t cache_offset;
    uint32_t cache_length;
    int cache_valid;
    uint8_t cache[NAOMI_FILE_CACHE_SIZE] __attribute__((aligned(4)));
} naomi_romfs_file_t;

typedef struct {
    uint32_t fs_offset;
    uint32_t root_offset;
    uint32_t root_entries;
    naomi_romfs_entry_t *root;
    int mounted;
} naomi_romfs_state_t;

static naomi_romfs_state_t romfs_state;

static uint32_t read_le32(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static int read_romfs_header(uint32_t offset, uint32_t *root_entries) {
    uint8_t header[NAOMI_ROMFS_HEADER_SIZE] __attribute__((aligned(4)));

    if (naomi_cart_read(header, offset, sizeof(header)) != 0)
        return -1;
    if (memcmp(header, "ROMFS\0", 6) != 0 ||
        header[6] != 1u || header[7] != 0u ||
        read_le32(header + 8) != NAOMI_ROMFS_CHECKSUM)
        return -1;

    *root_entries = read_le32(header + 12);
    return *root_entries ? 0 : -1;
}

static int find_romfs_in_table(const uint8_t *header, uint32_t table_offset,
                               uint32_t *fs_offset, uint32_t *root_entries) {
    for (uint32_t i = 0; i < NAOMI_SECTION_COUNT; ++i) {
        const uint8_t *section = header + table_offset + i * 12u;
        const uint32_t rom_offset = read_le32(section);
        const uint32_t section_size = read_le32(section + 8);
        uint32_t end;

        if (rom_offset == 0xFFFFFFFFu)
            break;
        if (!section_size || rom_offset >= NAOMI_CART_LIMIT ||
            section_size > NAOMI_CART_LIMIT - rom_offset)
            continue;

        end = rom_offset + section_size;
        end = (end + 3u) & ~3u;
        if (end >= NAOMI_CART_LIMIT)
            continue;
        if (read_romfs_header(end, root_entries) == 0) {
            *fs_offset = end;
            return 0;
        }
    }

    return -1;
}

static int locate_default_romfs(uint32_t *fs_offset, uint32_t *root_entries) {
    uint8_t header[NAOMI_HEADER_SIZE] __attribute__((aligned(4)));

    if (naomi_cart_read(header, 0, sizeof(header)) != 0)
        return -1;

    if (find_romfs_in_table(header, NAOMI_MAIN_SECTIONS,
                            fs_offset, root_entries) == 0)
        return 0;
    return find_romfs_in_table(header, NAOMI_TEST_SECTIONS,
                               fs_offset, root_entries);
}

static int load_directory(uint32_t offset, uint32_t entries,
                          naomi_romfs_entry_t **directory) {
    size_t bytes;

    if (!entries || entries > SIZE_MAX / sizeof(naomi_romfs_entry_t)) {
        errno = EOVERFLOW;
        return -1;
    }
    bytes = (size_t)entries * sizeof(naomi_romfs_entry_t);
    if (offset >= NAOMI_CART_LIMIT || bytes > NAOMI_CART_LIMIT - offset) {
        errno = EOVERFLOW;
        return -1;
    }

    naomi_romfs_entry_t *buffer = malloc(bytes);
    if (!buffer) {
        errno = ENOMEM;
        return -1;
    }
    if (naomi_cart_read(buffer, offset, bytes) != 0) {
        free(buffer);
        return -1;
    }

    *directory = buffer;
    return 0;
}

/*
 * Resolve a path against the ROMFS directory tree. Directory tables are read
 * one at a time, so nested lookups do not retain a chain of directory buffers.
 */
static int lookup_path(const char *path, naomi_romfs_info_t *info) {
    const naomi_romfs_entry_t *directory = romfs_state.root;
    naomi_romfs_entry_t *owned_directory = NULL;
    uint32_t directory_offset = romfs_state.root_offset;
    uint32_t directory_entries = romfs_state.root_entries;
    const char *cursor = path;

    while (*cursor == '/')
        ++cursor;
    if (!*cursor) {
        info->offset = directory_offset;
        info->size = directory_entries;
        info->type = NAOMI_ROMFS_TYPE_DIR;
        return 0;
    }

    while (*cursor) {
        char component[sizeof(((naomi_romfs_entry_t *)0)->name)];
        size_t component_length = 0;
        const naomi_romfs_entry_t *found = NULL;
        uint32_t entry_offset, entry_size, entry_type;
        int more;

        while (*cursor && *cursor != '/') {
            if (component_length + 1u >= sizeof(component)) {
                free(owned_directory);
                errno = ENAMETOOLONG;
                return -1;
            }
            component[component_length++] = *cursor++;
        }
        component[component_length] = '\0';
        while (*cursor == '/')
            ++cursor;
        more = *cursor != '\0';

        if (!component_length || !strcmp(component, "."))
            continue;
        if (!strcmp(component, "..")) {
            free(owned_directory);
            errno = EPERM;
            return -1;
        }

        for (uint32_t i = 0; i < directory_entries; ++i) {
            const naomi_romfs_entry_t *candidate = &directory[i];
            if (!strncmp(candidate->name, component, component_length) &&
                candidate->name[component_length] == '\0') {
                found = candidate;
                break;
            }
        }

        if (!found) {
            free(owned_directory);
            errno = ENOENT;
            return -1;
        }

        entry_offset = (uint32_t)found->offset;
        entry_size = found->size;
        entry_type = found->type;

        if (found->offset < 0) {
            free(owned_directory);
            errno = EINVAL;
            return -1;
        }

        if (more) {
            naomi_romfs_entry_t *child = NULL;
            uint32_t child_offset;

            if (entry_type != NAOMI_ROMFS_TYPE_DIR) {
                free(owned_directory);
                errno = ENOTDIR;
                return -1;
            }
            if (entry_offset > NAOMI_CART_LIMIT - directory_offset) {
                free(owned_directory);
                errno = EOVERFLOW;
                return -1;
            }
            child_offset = directory_offset + entry_offset;
            if (load_directory(child_offset, entry_size, &child) != 0) {
                free(owned_directory);
                return -1;
            }

            free(owned_directory);
            owned_directory = child;
            directory = child;
            directory_offset = child_offset;
            directory_entries = entry_size;
            continue;
        }

        if (entry_offset > NAOMI_CART_LIMIT - directory_offset) {
            free(owned_directory);
            errno = EOVERFLOW;
            return -1;
        }
        info->offset = directory_offset + entry_offset;
        info->size = entry_size;
        info->type = entry_type;
        free(owned_directory);

        if (info->type == NAOMI_ROMFS_TYPE_FILE &&
            (info->offset >= NAOMI_CART_LIMIT ||
             info->size > NAOMI_CART_LIMIT - info->offset)) {
            errno = EOVERFLOW;
            return -1;
        }
        return 0;
    }

    free(owned_directory);
    errno = EISDIR;
    return -1;
}

static void set_file_stat(struct stat *st, uint32_t size) {
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
    st->st_nlink = 1;
    st->st_size = (off_t)size;
}

static void *romfs_open(vfs_handler_t *vfs, const char *path, int mode) {
    naomi_romfs_info_t info;
    naomi_romfs_file_t *file;

    (void)vfs;
    if ((mode & O_MODE_MASK) != O_RDONLY) {
        errno = EROFS;
        return NULL;
    }
    if (mode & O_DIR) {
        errno = ENOTSUP;
        return NULL;
    }
    if (lookup_path(path, &info) != 0)
        return NULL;
    if (info.type != NAOMI_ROMFS_TYPE_FILE) {
        errno = EISDIR;
        return NULL;
    }

    file = calloc(1, sizeof(*file));
    if (!file) {
        errno = ENOMEM;
        return NULL;
    }
    file->image_offset = info.offset;
    file->size = info.size;
    return file;
}

static int romfs_close(void *handle) {
    if (!handle) {
        errno = EBADF;
        return -1;
    }
    free(handle);
    return 0;
}

static ssize_t romfs_read(void *handle, void *buffer, size_t count) {
    naomi_romfs_file_t *file = handle;
    uint8_t *destination = buffer;
    size_t done = 0;

    if (!file || (!buffer && count)) {
        errno = EBADF;
        return -1;
    }
    if (file->position >= file->size || !count)
        return 0;
    if (count > file->size - file->position)
        count = file->size - file->position;

    while (done < count) {
        const uint32_t absolute = file->image_offset + file->position;
        const uint32_t block = absolute & ~(NAOMI_FILE_CACHE_SIZE - 1u);
        size_t within = absolute - block;
        size_t amount;

        if (!file->cache_valid || file->cache_offset != block) {
            uint32_t available = NAOMI_CART_LIMIT - block;
            size_t block_length = available < NAOMI_FILE_CACHE_SIZE ?
                                  available : NAOMI_FILE_CACHE_SIZE;
            if (naomi_cart_read(file->cache, block, block_length) != 0)
                return done ? (ssize_t)done : -1;
            file->cache_offset = block;
            file->cache_length = (uint32_t)block_length;
            file->cache_valid = 1;
        }

        if (within >= file->cache_length) {
            errno = EIO;
            return done ? (ssize_t)done : -1;
        }
        amount = file->cache_length - within;
        if (amount > count - done)
            amount = count - done;

        memcpy(destination + done, file->cache + within, amount);
        done += amount;
        file->position += (uint32_t)amount;
    }

    return (ssize_t)done;
}

static off_t romfs_seek(void *handle, off_t offset, int whence) {
    naomi_romfs_file_t *file = handle;
    int64_t target;

    if (!file) {
        errno = EBADF;
        return -1;
    }

    switch (whence) {
        case SEEK_SET: target = offset; break;
        case SEEK_CUR: target = (int64_t)file->position + offset; break;
        case SEEK_END: target = (int64_t)file->size + offset; break;
        default:
            errno = EINVAL;
            return -1;
    }

    if (target < 0 || (uint64_t)target > file->size) {
        errno = EINVAL;
        return -1;
    }
    file->position = (uint32_t)target;
    return (off_t)file->position;
}

static off_t romfs_tell(void *handle) {
    naomi_romfs_file_t *file = handle;
    if (!file) {
        errno = EBADF;
        return -1;
    }
    return (off_t)file->position;
}

static size_t romfs_total(void *handle) {
    naomi_romfs_file_t *file = handle;
    if (!file) {
        errno = EBADF;
        return (size_t)-1;
    }
    return file->size;
}

static int romfs_stat(vfs_handler_t *vfs, const char *path,
                      struct stat *st, int flag) {
    naomi_romfs_info_t info;

    (void)vfs;
    (void)flag;
    if (!st) {
        errno = EINVAL;
        return -1;
    }
    if (lookup_path(path, &info) != 0)
        return -1;

    if (info.type == NAOMI_ROMFS_TYPE_DIR) {
        memset(st, 0, sizeof(*st));
        st->st_mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP |
                      S_IROTH | S_IXOTH;
        st->st_nlink = 2;
        st->st_size = -1;
        return 0;
    }
    if (info.type != NAOMI_ROMFS_TYPE_FILE) {
        errno = EIO;
        return -1;
    }

    set_file_stat(st, info.size);
    return 0;
}

static int romfs_fstat(void *handle, struct stat *st) {
    naomi_romfs_file_t *file = handle;
    if (!file || !st) {
        errno = EINVAL;
        return -1;
    }
    set_file_stat(st, file->size);
    return 0;
}

static vfs_handler_t romfs_vfs = {
    .nmmgr = {
        .pathname = "/naomi",
        .pid = 0,
        .version = 0x00010000,
        .flags = 0,
        .type = NMMGR_TYPE_VFS,
        .list_ent = NMMGR_LIST_INIT
    },
    .cache = 0,
    .privdata = &romfs_state,
    .open = romfs_open,
    .close = romfs_close,
    .read = romfs_read,
    .write = NULL,
    .seek = romfs_seek,
    .tell = romfs_tell,
    .total = romfs_total,
    .readdir = NULL,
    .ioctl = NULL,
    .rename = NULL,
    .unlink = NULL,
    .mmap = NULL,
    .complete = NULL,
    .stat = romfs_stat,
    .mkdir = NULL,
    .rmdir = NULL,
    .fcntl = NULL,
    .poll = NULL,
    .link = NULL,
    .symlink = NULL,
    .seek64 = NULL,
    .tell64 = NULL,
    .total64 = NULL,
    .readlink = NULL,
    .rewinddir = NULL,
    .fstat = romfs_fstat
};

int naomi_romfs_init_default(void) {
    uint32_t fs_offset, root_entries;
    naomi_romfs_entry_t *root = NULL;
    uint32_t root_offset;

    if (romfs_state.mounted)
        return 0;
    if (locate_default_romfs(&fs_offset, &root_entries) != 0) {
        errno = ENOENT;
        return -1;
    }
    if (fs_offset > NAOMI_CART_LIMIT - NAOMI_ROMFS_HEADER_SIZE) {
        errno = EOVERFLOW;
        return -1;
    }

    root_offset = fs_offset + NAOMI_ROMFS_HEADER_SIZE;
    if (load_directory(root_offset, root_entries, &root) != 0)
        return -1;

    romfs_state.fs_offset = fs_offset;
    romfs_state.root_offset = root_offset;
    romfs_state.root_entries = root_entries;
    romfs_state.root = root;

    if (nmmgr_handler_add(&romfs_vfs.nmmgr) != 0) {
        free(root);
        memset(&romfs_state, 0, sizeof(romfs_state));
        errno = EBUSY;
        return -1;
    }

    romfs_state.mounted = 1;
    return 0;
}

void naomi_romfs_shutdown(void) {
    if (!romfs_state.mounted)
        return;

    nmmgr_handler_remove(&romfs_vfs.nmmgr);
    free(romfs_state.root);
    memset(&romfs_state, 0, sizeof(romfs_state));
}
