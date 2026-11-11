#ifndef NAOMI_HOST_STUB_DC_MAPLE_H
#define NAOMI_HOST_STUB_DC_MAPLE_H
#include <stddef.h>
#include <stdint.h>
#define MAPLE_PORT_COUNT 4
#define MAPLE_FUNC_MIE 0x00000001u
#define MAPLE_FUNC_CONTROLLER 0x01000000u
typedef struct maple_device {
    uint8_t port;
    uint8_t unit;
} maple_device_t;
maple_device_t *maple_enum_type(size_t index, uint32_t function);
void *maple_dev_status(maple_device_t *device);
#endif
