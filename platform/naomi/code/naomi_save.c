/* Naomi's battery-backed SRAM save device. Keep this port in the cabinet's
 * reserved 0x6000-0x7fff window; the recompiled Daytona uses 0x1000-0x5fff
 * and Cannonball uses 0x0000-0x0fff. The file format is the same validated
 * VMS save image used by the Dreamcast VMU backend. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "save_store.h"
#include "naomi_save.h"
#include <dc/maple/mie.h>

#define SRAM_BASE ((volatile uint8_t *)0xA0206000u)
#define SRAM_BYTES 0x2000u
#define HEADER 16u
#define FILE_MAX (SRAM_BYTES - HEADER)
#define BLOCK 512u

static uint32_t checksum(const uint8_t *data, size_t size) {
    uint32_t sum = 0x44544e31u;
    for(size_t i = 0; i < size; ++i) sum = (sum << 5) + sum + data[i];
    return sum;
}

static uint32_t get32(size_t offset) {
    return (uint32_t)SRAM_BASE[offset] |
        (uint32_t)SRAM_BASE[offset + 1] << 8 |
        (uint32_t)SRAM_BASE[offset + 2] << 16 |
        (uint32_t)SRAM_BASE[offset + 3] << 24;
}

static void put32(size_t offset, uint32_t value) {
    for(unsigned i = 0; i < 4; ++i)
        SRAM_BASE[offset + i] = (uint8_t)(value >> (8 * i));
}

static size_t stored_size(void) {
    if(SRAM_BASE[0] != 'D' || SRAM_BASE[1] != 'T' ||
       SRAM_BASE[2] != 'N' || SRAM_BASE[3] != '1')
        return 0;
    const uint32_t size = get32(4);
    if(!size || size > FILE_MAX) return 0;
    uint32_t sum = 0x44544e31u;
    for(size_t i = 0; i < size; ++i)
        sum = (sum << 5) + sum + SRAM_BASE[HEADER + i];
    return sum == get32(8) ? size : 0;
}

static void *sram_find(void *context) {
    (void)context;
    return (void *)SRAM_BASE;
}

static int sram_free_blocks(void *context, void *card) {
    (void)context; (void)card;
    const size_t used = (stored_size() + BLOCK - 1) / BLOCK;
    return (int)(FILE_MAX / BLOCK - used);
}

static int sram_file_blocks(void *context, void *card, const char *name) {
    (void)context; (void)card; (void)name;
    return (int)((stored_size() + BLOCK - 1) / BLOCK);
}

static int sram_read(void *context, void *card, const char *name,
                     uint8_t **data, size_t *size) {
    (void)context; (void)card; (void)name;
    const size_t bytes = stored_size();
    if(!bytes) return -1;
    uint8_t *buffer = malloc(bytes);
    if(!buffer) return -1;
    for(size_t i = 0; i < bytes; ++i)
        buffer[i] = SRAM_BASE[HEADER + i];
    if(checksum(buffer, bytes) != get32(8)) {
        free(buffer);
        return -1;
    }
    *data = buffer;
    *size = bytes;
    return 0;
}

static void sram_release(void *context, uint8_t *data) {
    (void)context;
    free(data);
}

static int sram_write(void *context, void *card, const char *name,
                      const uint8_t *data, size_t size) {
    (void)context; (void)card; (void)name;
    if(!size || size > FILE_MAX) return -1;

    /* Invalidate first; a power loss cannot leave a partially-written save
     * looking valid. Write and verify the payload before publishing magic. */
    SRAM_BASE[0] = 0;
    for(size_t i = 0; i < size; ++i)
        SRAM_BASE[HEADER + i] = data[i];
    put32(4, (uint32_t)size);
    put32(8, checksum(data, size));
    put32(12, 0);

    for(size_t i = 0; i < size; ++i)
        if(SRAM_BASE[HEADER + i] != data[i]) return -1;

    SRAM_BASE[3] = '1';
    SRAM_BASE[2] = 'N';
    SRAM_BASE[1] = 'T';
    SRAM_BASE[0] = 'D';
    return 0;
}

#define EEPROM_SETTINGS_SIZE 9u
#define EEPROM_SETTINGS_OFFSET 44u
#define EEPROM_SETTINGS_LENGTH_OFFSET 39u
static const uint8_t eeprom_magic[4] = {'D','U','S','A'};

static uint16_t eeprom_get16(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)p[1] << 8;
}

static int eeprom_game_block(uint8_t *eeprom, size_t *size_out) {
    const size_t size = eeprom[EEPROM_SETTINGS_LENGTH_OFFSET];
    if(size == 0 || size > MIE_EEPROM_SIZE - EEPROM_SETTINGS_OFFSET) return 0;
    if(size < EEPROM_SETTINGS_SIZE || memcmp(eeprom + EEPROM_SETTINGS_OFFSET,eeprom_magic,4)) return 0;
    const uint16_t crc = mie_eeprom_crc16(eeprom + EEPROM_SETTINGS_OFFSET,size);
    if(eeprom_get16(eeprom + 36) != crc || eeprom_get16(eeprom + 40) != crc) return 0;
    if(eeprom[EEPROM_SETTINGS_OFFSET + 4] != 1) return 0;
    *size_out = size;
    return 1;
}

int nm_naomi_eeprom_load_settings(nm_settings_t *settings) {
    if(!settings) return 0;
    uint8_t eeprom[MIE_EEPROM_SIZE];
    if(!mie_get_eeprom(eeprom)) return 0;
    size_t size = 0;
    if(!eeprom_game_block(eeprom,&size)) return 0;
    (void)size;
    nm_settings_t value = {
        .difficulty = eeprom[EEPROM_SETTINGS_OFFSET + 5],
        .advertise_sound = eeprom[EEPROM_SETTINGS_OFFSET + 6],
        .control_scheme = eeprom[EEPROM_SETTINGS_OFFSET + 7],
        .reset_with_p1_d = eeprom[EEPROM_SETTINGS_OFFSET + 8]
    };
    if(!nm_settings_valid(&value)) return 0;
    *settings = value;
    return 1;
}

int nm_naomi_eeprom_store_settings(const nm_settings_t *settings) {
    if(!nm_settings_valid(settings)) return 0;
    uint8_t eeprom[MIE_EEPROM_SIZE];
    if(!mie_get_eeprom(eeprom)) return 0;

    /* Never replace another title's game-settings block. An empty block or
     * our own DUSA block is safe to update; the system-settings bytes remain
     * untouched and their CRC is checked before any write. */
    const uint16_t system_crc = mie_eeprom_crc16(eeprom + 2,16);
    if(eeprom_get16(eeprom) != system_crc || eeprom_get16(eeprom + 18) != system_crc)
        return 0;

    size_t existing_size = 0;
    const size_t declared_size = eeprom[EEPROM_SETTINGS_LENGTH_OFFSET];
    if(declared_size != 0) {
        if(!eeprom_game_block(eeprom,&existing_size)) return 0;
    } else {
        existing_size = EEPROM_SETTINGS_SIZE;
        eeprom[EEPROM_SETTINGS_LENGTH_OFFSET] = EEPROM_SETTINGS_SIZE;
    }

    uint8_t *data = eeprom + EEPROM_SETTINGS_OFFSET;
    memcpy(data,eeprom_magic,sizeof(eeprom_magic));
    data[4] = 1;
    data[5] = settings->difficulty;
    data[6] = settings->advertise_sound;
    data[7] = settings->control_scheme;
    data[8] = settings->reset_with_p1_d;
    (void)existing_size;

    mie_eeprom_fix_crc(eeprom);
    if(!mie_set_eeprom(eeprom)) return 0;

    uint8_t verify[MIE_EEPROM_SIZE];
    if(!mie_get_eeprom(verify)) return 0;
    size_t verify_size = 0;
    if(!eeprom_game_block(verify,&verify_size)) return 0;
    (void)verify_size;
    const nm_settings_t readback = {
        .difficulty = verify[EEPROM_SETTINGS_OFFSET + 5],
        .advertise_sound = verify[EEPROM_SETTINGS_OFFSET + 6],
        .control_scheme = verify[EEPROM_SETTINGS_OFFSET + 7],
        .reset_with_p1_d = verify[EEPROM_SETTINGS_OFFSET + 8]
    };
    return nm_settings_valid(&readback) && !memcmp(&readback,settings,sizeof(readback));
}

static const nm_save_device_t device = {
    NULL, sram_find, sram_free_blocks, sram_file_blocks,
    sram_read, sram_release, sram_write
};

const nm_save_device_t *nm_naomi_save_device(void) {
    return &device;
}
