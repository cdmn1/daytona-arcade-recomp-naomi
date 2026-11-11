/*
 * ROM-board / NetDIMM PIO diagnostic.
 * This reads the Naomi ROM header without copying the game image into RAM.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <kos.h>
#include <kos/init.h>

#include "naomi_cart.h"

KOS_INIT_FLAGS(INIT_DEFAULT);

#define NAOMI_HEADER_SIZE 0x500u
#define NAOMI_MAIN_SEGMENTS_OFFSET 0x360u
#define NAOMI_ENTRY_OFFSET 0x420u
#define NAOMI_ROM_DATA_OFFSET 0x1000u

static uint32_t read_u32(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

int main(void) {
    static uint32_t header_words[NAOMI_HEADER_SIZE / sizeof(uint32_t)];
    uint8_t *header = (uint8_t *)header_words;
    uint8_t fs_header[16] __attribute__((aligned(4)));

    puts("Daytona Naomi ROM-board / NetDIMM probe");
#if defined(_arch_sub_naomi)
    puts("KOS sub-architecture: Naomi");
#else
    puts("ERROR: this executable was not compiled for Naomi");
    for (;;) thd_sleep(1000);
#endif

    if (naomi_cart_read(header, 0, NAOMI_HEADER_SIZE) != 0) {
        printf("ERROR: ROM-board PIO read failed (errno=%d)\n", errno);
        for (;;) thd_sleep(1000);
    }

    if (memcmp(header, "NAOMI           ", 16) != 0 &&
        memcmp(header, "Naomi2          ", 16) != 0) {
        printf("ERROR: unexpected ROM signature: %.16s\n", header);
        for (;;) thd_sleep(1000);
    }

    printf("ROM signature: %.16s\n", header);
    printf("Publisher: %.32s\n", header + 0x10);
    printf("Title: %.32s\n", header + 0x50);
    printf("Entry point: %08lx\n",
           (unsigned long)read_u32(header + NAOMI_ENTRY_OFFSET));

    uint32_t rom_offset = read_u32(header + NAOMI_MAIN_SEGMENTS_OFFSET);
    uint32_t load_address = read_u32(header + NAOMI_MAIN_SEGMENTS_OFFSET + 4);
    uint32_t segment_size = read_u32(header + NAOMI_MAIN_SEGMENTS_OFFSET + 8);

    if (rom_offset == 0xFFFFFFFFu || segment_size == 0) {
        puts("ERROR: no main executable segment found");
        for (;;) thd_sleep(1000);
    }

    printf("Main segment: ROM %08lx, RAM %08lx, size %lu bytes\n",
           (unsigned long)rom_offset, (unsigned long)load_address,
           (unsigned long)segment_size);

    uint32_t fs_offset = (rom_offset + segment_size + 3u) & ~3u;
    if (naomi_cart_read(fs_header, fs_offset, sizeof(fs_header)) != 0) {
        printf("ROMFS probe outside PIO range or read failed (errno=%d)\n", errno);
    }
    else if (memcmp(fs_header, "ROMFS\0", 6) == 0) {
        printf("ROMFS found at %08lx (version %u.%u)\n",
               (unsigned long)fs_offset, (unsigned)fs_header[6],
               (unsigned)fs_header[7]);
    }
    else {
        printf("No appended ROMFS at %08lx (magic %.6s)\n",
               (unsigned long)fs_offset, fs_header);
    }

    puts("PIO probe complete; no bulk image data was copied into Naomi RAM.");
    for (;;) thd_sleep(1000);
    return 0;
}
