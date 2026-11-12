/* Naomi EEPROM backend: read-only identity/CRC validation, selective GAME ASSIGNMENTS writes. */
#undef NDEBUG
#include "game_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dc/maple/mie.h>

static uint8_t hardware_eeprom[MIE_EEPROM_SIZE];
static unsigned reads,writes;
static int read_ok=1,write_ok=1;
bool mie_get_eeprom(void *dst) {
    ++reads;
    if(!read_ok) return false;
    memcpy(dst,hardware_eeprom,sizeof(hardware_eeprom));
    return true;
}
bool mie_set_eeprom(uint8_t *src) {
    ++writes;
    if(!write_ok) return false;
    memcpy(hardware_eeprom,src,sizeof(hardware_eeprom));
    return true;
}
static uint16_t crc16_xmodem(const uint8_t *p) {
    uint16_t crc=0;
    for(unsigned i=0x0a;i<0x80;++i) {
        crc^=(uint16_t)p[i]<<8;
        for(unsigned b=0;b<8;++b) crc=(uint16_t)(crc&0x8000u?(crc<<1)^0x1021u:crc<<1);
    }
    return crc;
}
static void seal(void) {
    const uint16_t crc=crc16_xmodem(hardware_eeprom);
    hardware_eeprom[8]=(uint8_t)crc;hardware_eeprom[9]=(uint8_t)(crc>>8);
}
int main(void) {
    nm_race_record_seed_t seeds[NM_SAVE_TABLES][NM_SAVE_SLOTS];
    for(unsigned t=0;t<NM_SAVE_TABLES;++t) for(unsigned i=0;i<NM_SAVE_SLOTS;++i)
        seeds[t][i]=(nm_race_record_seed_t){.rank=i+1,.time=1000+i,.name=0x414141};
    memset(hardware_eeprom,0x5a,sizeof(hardware_eeprom));
    memcpy(hardware_eeprom,"SEGA",4);
    hardware_eeprom[4]=0x82;hardware_eeprom[5]=0x40;hardware_eeprom[6]=0;hardware_eeprom[7]=1;
    hardware_eeprom[0x19]=1;hardware_eeprom[0x20]=2;seal();
    const uint8_t untouched_10=hardware_eeprom[0x10],untouched_1a=hardware_eeprom[0x1a],
        untouched_1b=hardware_eeprom[0x1b],untouched_7c=hardware_eeprom[0x7c];
    nm_game_save_boot(seeds,1);
    assert(nm_game_save_status().boot==NM_SAVE_OK);
    assert(nm_game_settings()->difficulty==NM_DIFFICULTY_HARD &&
        nm_game_settings()->advertise_sound==NM_ADVERTISE_OFF);
    assert(nm_game_settings()->control_scheme==NM_CONTROL_ANALOG &&
        nm_game_settings()->reset_with_p1_d==NM_RESET_BUTTON_ON);
    nm_settings_t changed=*nm_game_settings();
    changed.difficulty=NM_DIFFICULTY_EASY;changed.advertise_sound=NM_ADVERTISE_ON;
    changed.control_scheme=NM_CONTROL_DIGITAL;changed.reset_with_p1_d=NM_RESET_BUTTON_OFF;
    assert(nm_game_settings_set(&changed));
    nm_game_save_request();
    assert(nm_game_save_status().last==NM_SAVE_OK && writes==1);
    assert(hardware_eeprom[0x19]==NM_ADVERTISE_ON && hardware_eeprom[0x20]==NM_DIFFICULTY_EASY);
    assert(hardware_eeprom[0x10]==untouched_10 && hardware_eeprom[0x1a]==untouched_1a &&
        hardware_eeprom[0x1b]==untouched_1b && hardware_eeprom[0x7c]==untouched_7c);
    assert((uint16_t)(hardware_eeprom[8]|(uint16_t)hardware_eeprom[9]<<8)==crc16_xmodem(hardware_eeprom));
    /* A foreign/blank EEPROM is never initialised or overwritten. */
    hardware_eeprom[0]='X';const uint8_t before[MIE_EEPROM_SIZE]={0};
    uint8_t snapshot[MIE_EEPROM_SIZE];memcpy(snapshot,hardware_eeprom,sizeof(snapshot));
    nm_game_save_boot(seeds,1);
    assert(nm_game_save_status().boot==NM_SAVE_DAMAGED);
    changed=*nm_game_settings();changed.difficulty=NM_DIFFICULTY_HARDEST;
    assert(nm_game_settings_set(&changed));nm_game_save_request();
    assert(nm_game_save_status().last==NM_SAVE_DAMAGED && writes==1 &&
        !memcmp(snapshot,hardware_eeprom,sizeof(snapshot)));
    (void)before;
    /* A transport error is reported and never mistaken for a valid save. */
    read_ok=0;nm_game_save_boot(seeds,1);
    assert(nm_game_save_status().boot==NM_SAVE_IO_ERROR);
    puts("PASS: Naomi Daytona EEPROM identity, CRC, selective settings writes and safe rejection");
    return 0;
}
