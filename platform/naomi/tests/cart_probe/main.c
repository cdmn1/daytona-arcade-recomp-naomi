/*
 * ROM-board / NetDIMM PIO and ROMFS diagnostic.
 * The test payload remains in the resident image and is read on demand.
 */
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <kos.h>
#include <kos/init.h>

#include "naomi_cart.h"
#include "naomi_romfs.h"

KOS_INIT_FLAGS(INIT_DEFAULT);

#define NAOMI_HEADER_SIZE 0x500u
#define NAOMI_MAIN_SEGMENTS_OFFSET 0x360u
#define NAOMI_ENTRY_OFFSET 0x420u

static uint32_t read_u32(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

int main(void) {
    static uint32_t header_words[NAOMI_HEADER_SIZE / sizeof(uint32_t)];
    uint8_t *header = (uint8_t *)header_words;
    char payload[80];

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

    if (naomi_romfs_init_default() != 0) {
        printf("ERROR: could not mount appended ROMFS (errno=%d)\n", errno);
        for (;;) thd_sleep(1000);
    }
    puts("ROMFS mounted at /naomi");

    FILE *file = fopen("/naomi/probe.txt", "rb");
    if (!file) {
        printf("ERROR: could not open /naomi/probe.txt (errno=%d)\n", errno);
        for (;;) thd_sleep(1000);
    }

    if (!fgets(payload, sizeof(payload), file)) {
        printf("ERROR: could not read ROMFS test file (errno=%d)\n", errno);
        fclose(file);
        for (;;) thd_sleep(1000);
    }
    fclose(file);

    if (strcmp(payload, "Daytona Naomi NetDIMM ROMFS probe OK\n") != 0) {
        printf("ERROR: unexpected ROMFS payload: %s\n", payload);
        for (;;) thd_sleep(1000);
    }

    printf("ROMFS payload verified: %s", payload);
    puts("PIO + ROMFS probe complete; bulk game assets remain in NetDIMM RAM.");
    for (;;) thd_sleep(1000);
    return 0;
}
