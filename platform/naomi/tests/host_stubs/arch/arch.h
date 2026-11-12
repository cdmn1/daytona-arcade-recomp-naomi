#ifndef NAOMI_HOST_STUB_ARCH_ARCH_H
#define NAOMI_HOST_STUB_ARCH_ARCH_H

/* Minimal declarations used by the standalone Naomi input probe's host
 * syntax-only check. These are not substitutes for the Naomi KOS headers. */
#define HW_TYPE_NAOMI  0x10
#define HW_TYPE_RETAIL 0x00
#define HW_TYPE_SET5   0x20
int hardware_sys_mode(int *region);

#endif
