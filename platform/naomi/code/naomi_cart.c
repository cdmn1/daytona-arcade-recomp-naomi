/*
 * Naomi ROM-board PIO access, adapted from the public-domain cart reader in
 * DragonMinded/libnaomi (libnaomi/cart.c).
 */
#include "naomi_cart.h"

#include <errno.h>
#include <kos/irq.h>
#include <stdint.h>

#define NAOMI_CART_OFFSET_HIGH (*(volatile uint16_t *)0xA05F7000u)
#define NAOMI_CART_OFFSET_LOW  (*(volatile uint16_t *)0xA05F7004u)
#define NAOMI_CART_DATA        (*(volatile uint16_t *)0xA05F7008u)

/* The PIO offset register exposes 12 address bits above bit 15. */
#define NAOMI_CART_PIO_LIMIT   0x10000000u
#define NAOMI_CART_READ_CHUNK  1024u

int naomi_cart_read(void *destination, uint32_t source_offset, size_t length) {
    if ((!destination && length) ||
        (((uintptr_t)destination & 1u) != 0) ||
        ((source_offset & 1u) != 0) ||
        ((length & 1u) != 0)) {
        errno = EINVAL;
        return -1;
    }

    if (source_offset >= NAOMI_CART_PIO_LIMIT ||
        length > (size_t)(NAOMI_CART_PIO_LIMIT - source_offset)) {
        errno = EOVERFLOW;
        return -1;
    }

    uint16_t *out = (uint16_t *)destination;

    while (length) {
        size_t chunk = length;
        if (chunk > NAOMI_CART_READ_CHUNK)
            chunk = NAOMI_CART_READ_CHUNK;

        /*
         * The ROM-board address/data registers are shared hardware state.
         * Keep each bounded transfer atomic with respect to interrupts and
         * other KOS activity that might touch the same registers.
         */
        irq_mask_t irq_state = irq_disable();

        NAOMI_CART_OFFSET_LOW = (uint16_t)(source_offset & 0xFFFFu);
        NAOMI_CART_OFFSET_HIGH =
            (uint16_t)(0x8000u | ((source_offset >> 16) & 0x0FFFu));

        for (size_t i = 0; i < chunk / 2u; ++i)
            out[i] = NAOMI_CART_DATA;

        irq_restore(irq_state);

        out += chunk / 2u;
        source_offset += (uint32_t)chunk;
        length -= chunk;
    }

    return 0;
}
