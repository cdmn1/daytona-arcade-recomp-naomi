#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <kos/fs.h>
#include <kos/nmmgr.h>
#include "naomi_cart.h"
#include "naomi_romfs.h"

#define IMAGE_SIZE 0x1800u
#define FS_OFFSET  0x1020u
#define ROOT_OFFSET (FS_OFFSET + 16u)
#define DIR_BYTES  (3u * 256u)

static uint8_t image[IMAGE_SIZE] __attribute__((aligned(4)));
static vfs_handler_t *mounted_vfs;

static void put_u32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void put_entry(uint8_t *p, int32_t offset, uint32_t size,
                      uint32_t type, const char *name) {
    put_u32(p, (uint32_t)offset);
    put_u32(p + 4, size);
    put_u32(p + 8, type);
    memset(p + 12, 0, 244);
    strncpy((char *)p + 12, name, 243);
}

int naomi_cart_read(void *destination, uint32_t source_offset, size_t length) {
    if (!destination || ((uintptr_t)destination & 1u) ||
        (source_offset & 1u) || (length & 1u) ||
        source_offset > IMAGE_SIZE || length > IMAGE_SIZE - source_offset) {
        errno = EIO;
        return -1;
    }
    memcpy(destination, image + source_offset, length);
    return 0;
}

int nmmgr_handler_add(nmmgr_handler_t *handler) {
    mounted_vfs = (vfs_handler_t *)handler;
    return 0;
}

int nmmgr_handler_remove(nmmgr_handler_t *handler) {
    assert((vfs_handler_t *)handler == mounted_vfs);
    mounted_vfs = NULL;
    return 0;
}

static void build_test_image(void) {
    static const char payload[] = "Naomi ROMFS nested read OK\n";
    uint8_t *root = image + ROOT_OFFSET;
    uint8_t *music = root + DIR_BYTES;
    uint8_t *file_data = music + DIR_BYTES;

    memcpy(image, "NAOMI           ", 16);
    put_u32(image + 0x360, 0x1000u);
    put_u32(image + 0x364, 0x8C020000u);
    put_u32(image + 0x368, 0x20u);
    put_u32(image + 0x36C, 0xFFFFFFFFu);
    put_u32(image + 0x420, 0x8C020000u);

    memcpy(image + FS_OFFSET, "ROMFS\0", 6);
    image[FS_OFFSET + 6] = 1;
    image[FS_OFFSET + 7] = 0;
    put_u32(image + FS_OFFSET + 8, 0x11291985u);
    put_u32(image + FS_OFFSET + 12, 3);

    put_entry(root, 0, 3, 1, ".");
    put_entry(root + 256, 0, 3, 1, "..");
    put_entry(root + 512, DIR_BYTES, 3, 1, "music");

    put_entry(music, 0, 3, 1, ".");
    put_entry(music + 256, -(int32_t)DIR_BYTES, 3, 1, "..");
    put_entry(music + 512, DIR_BYTES, sizeof(payload) - 1, 2, "probe.txt");
    memcpy(file_data, payload, sizeof(payload) - 1);
}

int main(void) {
    char buffer[64] = {0};
    struct stat st;
    const char expected[] = "Naomi ROMFS nested read OK\n";

    build_test_image();
    assert(naomi_romfs_init_default() == 0);
    assert(mounted_vfs != NULL);

    void *file = mounted_vfs->open(mounted_vfs, "/music/probe.txt", O_RDONLY);
    assert(file != NULL);

    ssize_t count = mounted_vfs->read(file, buffer, sizeof(buffer) - 1);
    assert(count == (ssize_t)(sizeof(expected) - 1));
    assert(strcmp(buffer, expected) == 0);

    assert(mounted_vfs->seek(file, 0, SEEK_SET) == 0);
    memset(buffer, 0, sizeof(buffer));
    count = mounted_vfs->read(file, buffer, sizeof(buffer) - 1);
    assert(count == (ssize_t)(sizeof(expected) - 1));
    assert(strcmp(buffer, expected) == 0);

    assert(mounted_vfs->stat(mounted_vfs, "/music/probe.txt", &st, 0) == 0);
    assert(st.st_size == (off_t)(sizeof(expected) - 1));
    assert(mounted_vfs->fstat(file, &st) == 0);
    assert(st.st_size == (off_t)(sizeof(expected) - 1));

    assert(mounted_vfs->close(file) == 0);
    naomi_romfs_shutdown();
    assert(mounted_vfs == NULL);

    puts("Naomi ROMFS VFS host test passed.");
    return 0;
}
