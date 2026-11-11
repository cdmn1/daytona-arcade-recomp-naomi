#ifndef DAYTONA_NAOMI_CART_H
#define DAYTONA_NAOMI_CART_H

#include <stddef.h>
#include <stdint.h>

/*
 * Read bytes from the Naomi ROM-board / NetDIMM image using the PIO window.
 *
 * The source offset, destination pointer and length must be 2-byte aligned.
 * This first implementation addresses the low 256 MiB of the image; a
 * separate DMA implementation is needed to address beyond that range.
 *
 * Returns 0 on success, -1 on invalid arguments or an out-of-range request.
 */
int naomi_cart_read(void *destination, uint32_t source_offset, size_t length);

#endif
