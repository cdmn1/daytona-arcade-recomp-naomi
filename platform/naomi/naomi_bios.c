#include "naomi_bios.h"

#include <stdint.h>

#define NAOMI_BIOS_HELPER_ROM_ADDR  0xA0060000u
#define NAOMI_BIOS_HELPER_RAM_ADDR  0xAC018000u
#define NAOMI_BIOS_HELPER_BYTES     0x7000u
#define NAOMI_BIOS_HELPER_WORDS     (NAOMI_BIOS_HELPER_BYTES / 4u)
#define NAOMI_BIOS_HELPER_TEST_OFF  0x44u
#define NAOMI_BIOS_HELPER_MASK      0x0FFF0000u

static bool helpers_ready;
static bool previous_test_pressed;
static bool test_switch_initialized;

bool naomi_bios_helpers_init(void)
{
    volatile const uint32_t *rom =
        (volatile const uint32_t *)NAOMI_BIOS_HELPER_ROM_ADDR;
    volatile uint32_t *ram =
        (volatile uint32_t *)NAOMI_BIOS_HELPER_RAM_ADDR;
    const uint32_t expected_region =
        NAOMI_BIOS_HELPER_RAM_ADDR & NAOMI_BIOS_HELPER_MASK;
    uint32_t i;

    if(helpers_ready) {
        return true;
    }

    /* Retail VF4 validates the first eight table entries before copying. */
    for(i = 0; i < 8; ++i) {
        if((rom[i] & NAOMI_BIOS_HELPER_MASK) != expected_region) {
            return false;
        }
    }

    for(i = 0; i < NAOMI_BIOS_HELPER_WORDS; ++i) {
        ram[i] = rom[i];
    }

    if((ram[NAOMI_BIOS_HELPER_TEST_OFF / 4u] &
        NAOMI_BIOS_HELPER_MASK) != expected_region) {
        return false;
    }

    helpers_ready = true;
    return true;
}

bool naomi_bios_helpers_ready(void)
{
    return helpers_ready;
}

bool naomi_bios_test_switch_update(bool pressed)
{
    bool edge;

    if(!test_switch_initialized) {
        previous_test_pressed = pressed;
        test_switch_initialized = true;
        return false;
    }

    edge = pressed && !previous_test_pressed;
    previous_test_pressed = pressed;
    return edge;
}

void naomi_bios_enter_system_test(void)
{
    volatile uint32_t *table =
        (volatile uint32_t *)NAOMI_BIOS_HELPER_RAM_ADDR;
    uint32_t helper;

    if(!helpers_ready) {
        for(;;) {
        }
    }

    helper = table[NAOMI_BIOS_HELPER_TEST_OFF / 4u];

    /* Retail BIOS helper pointers are called through their uncached P2 alias. */
    ((void (*)(void))(uintptr_t)(helper - 0x60000000u))();

    for(;;) {
    }
}
