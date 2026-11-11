#ifndef NAOMI_HOST_STUB_KOS_NMMGR_H
#define NAOMI_HOST_STUB_KOS_NMMGR_H
#include <stdint.h>
#include <stddef.h>
#define NAME_MAX 256
#define NMMGR_TYPE_VFS 0x0010
#define NMMGR_LIST_INIT { NULL }
typedef struct nmmgr_handler {
    char pathname[NAME_MAX];
    int pid;
    uint32_t version;
    uint32_t flags;
    uint32_t type;
    struct { struct nmmgr_handler *sle_next; } list_ent;
} nmmgr_handler_t;
int nmmgr_handler_add(nmmgr_handler_t *handler);
int nmmgr_handler_remove(nmmgr_handler_t *handler);
#endif
