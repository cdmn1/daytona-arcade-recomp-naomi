#ifndef NAOMI_BIOS_H
#define NAOMI_BIOS_H

#include <stdbool.h>

/* Call once during Naomi application startup before using BIOS helper 0x44. */
bool naomi_bios_helpers_init(void);
bool naomi_bios_helpers_ready(void);

/* Returns true once for each rising edge of the physical JVS system TEST switch. */
bool naomi_bios_test_switch_update(bool pressed);

/* Transfers control to the Naomi BIOS System Test Menu. Does not return. */
#if defined(__GNUC__)
__attribute__((noreturn))
#endif
void naomi_bios_enter_system_test(void);

#endif
