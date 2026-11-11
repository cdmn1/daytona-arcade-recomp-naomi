#ifndef NAOMI_HOST_STUB_KOS_FS_H
#define NAOMI_HOST_STUB_KOS_FS_H
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <kos/nmmgr.h>
#define O_MODE_MASK 0x0f
#define O_DIR 0x1000
typedef struct vfs_handler {
    nmmgr_handler_t nmmgr;
    int cache;
    void *privdata;
    void *(*open)(struct vfs_handler *, const char *, int);
    int (*close)(void *);
    ssize_t (*read)(void *, void *, size_t);
    ssize_t (*write)(void *, const void *, size_t);
    off_t (*seek)(void *, off_t, int);
    off_t (*tell)(void *);
    size_t (*total)(void *);
    const void *(*readdir)(void *);
    int (*ioctl)(void *, int, va_list);
    int (*rename)(struct vfs_handler *, const char *, const char *);
    int (*unlink)(struct vfs_handler *, const char *);
    void *(*mmap)(void *);
    int (*complete)(void *, ssize_t *);
    int (*stat)(struct vfs_handler *, const char *, struct stat *, int);
    int (*mkdir)(struct vfs_handler *, const char *);
    int (*rmdir)(struct vfs_handler *, const char *);
    int (*fcntl)(void *, int, va_list);
    short (*poll)(void *, short);
    int (*link)(struct vfs_handler *, const char *, const char *);
    int (*symlink)(struct vfs_handler *, const char *, const char *);
    int64_t (*seek64)(void *, int64_t, int);
    int64_t (*tell64)(void *);
    uint64_t (*total64)(void *);
    ssize_t (*readlink)(struct vfs_handler *, const char *, char *, size_t);
    int (*rewinddir)(void *);
    int (*fstat)(void *, struct stat *);
} vfs_handler_t;
#endif
