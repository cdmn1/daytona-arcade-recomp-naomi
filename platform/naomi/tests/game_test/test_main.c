/*
 * Daytona USA's separate Naomi GAME TEST MODE program.
 * Entered through the Naomi ROM header's test entry, not from the title loop.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kos.h>
#include <arch/arch.h>
#include <dc/minifont.h>
#include <dc/maple.h>
#include <dc/maple/mie.h>
#include <dc/pvr.h>

#include "save_data.h"
#include "save_store.h"
#include "naomi_save.h"
#include "naomi_romfs.h"
#include "audio/sound.h"
#include "race_record_data.h"

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MIE);

#define LINE_COUNT 12
#define LINE_BYTES 64
#define FONT_W 12
#define FONT_H 24

static pvr_ptr_t font_tex;
static nm_save_t save;
static int dirty;
static int save_failed;
static int eeprom_warning;
static int sound_ready;
static unsigned sound_selected;

enum { PAGE_MENU, PAGE_INPUT, PAGE_ASSIGN, PAGE_BOOKKEEPING, PAGE_SOUND, PAGE_BGM, PAGE_SE, PAGE_VOICE, PAGE_CLEAR };
enum { MENU_INPUT, MENU_ASSIGN, MENU_BOOKKEEPING, MENU_SOUND, MENU_CLEAR, MENU_EXIT, MENU_COUNT };
enum { SOUND_BGM, SOUND_SE, SOUND_VOICE, SOUND_STOP, SOUND_EXIT, SOUND_COUNT };
enum { ASSIGN_DIFFICULTY, ASSIGN_ADVERTISE, ASSIGN_COIN_MODE, ASSIGN_CONTROL, ASSIGN_COUNTRY, ASSIGN_CABINET, ASSIGN_EXIT, ASSIGN_COUNT };

static mie_state_t *mie_status(void) {
    maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_MIE);
    return dev ? (mie_state_t *)maple_dev_status(dev) : NULL;
}

static void font_init(void) {
    uint16_t *vram = (uint16_t *)font_tex;
    for(int y = 0; y < 8; ++y) {
        for(int x = 0; x < 16; ++x) {
            minifont_draw(vram, 256, (uint32_t)(y * 16 + x));
            vram += 16;
        }
        vram += 23 * 256;
    }
}

static void draw_char(float x, float y, int c) {
    if(c <= ' ' || c >= 127) return;
    pvr_vertex_t v;
    const int ix = (c % 16) * 16, iy = (c / 16) * 24;
    const float u1 = ix / 256.0f, v1 = iy / 256.0f;
    const float u2 = (ix + FONT_W) / 256.0f, v2 = (iy + FONT_H) / 256.0f;
    v.flags=PVR_CMD_VERTEX;v.x=x;v.y=y+FONT_H;v.z=1.0f;v.u=u1;v.v=v2;
    v.argb=PVR_PACK_COLOR(1,1,1,1);v.oargb=0;pvr_prim(&v,sizeof(v));
    v.x=x;v.y=y;v.u=u1;v.v=v1;pvr_prim(&v,sizeof(v));
    v.x=x+FONT_W;v.y=y+FONT_H;v.u=u2;v.v=v2;pvr_prim(&v,sizeof(v));
    v.flags=PVR_CMD_VERTEX_EOL;v.x=x+FONT_W;v.y=y;v.u=u2;v.v=v1;pvr_prim(&v,sizeof(v));
}

static void draw_lines(char lines[LINE_COUNT][LINE_BYTES]) {
    pvr_wait_ready();
    pvr_scene_begin();
    pvr_list_begin(PVR_LIST_TR_POLY);
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t poly;
    pvr_poly_cxt_txr(&cxt,PVR_LIST_TR_POLY,
        PVR_TXRFMT_ARGB1555|PVR_TXRFMT_NONTWIDDLED,256,256,font_tex,PVR_FILTER_NONE);
    pvr_poly_compile(&poly,&cxt);
    pvr_prim(&poly,sizeof(poly));
    for(int row=0;row<LINE_COUNT;++row) {
        float x=8.0f;
        for(size_t i=0;lines[row][i];++i,x+=FONT_W)
            draw_char(x,8.0f+row*FONT_H,(unsigned char)lines[row][i]);
    }
    pvr_list_finish();
    pvr_scene_finish();
}

static void exit_to_system_menu(void) {
    arch_set_exit_path(ARCH_EXIT_MENU);
    exit(0);
}

static int save_now(void) {
    uint8_t file[NM_SAVE_FILE_BYTES];
    uint8_t scratch[NM_SAVE_FILE_BLOCKS * 512u];
    const nm_save_device_t *device=nm_naomi_save_device();
    const int eeprom_ok=nm_naomi_eeprom_store_settings(&save.settings);
    eeprom_warning=!eeprom_ok;
    if(nm_save_file_build(&save,file)!=NM_SAVE_FILE_BYTES) return 0;
    const int sram_ok=nm_save_store_write(device,file,NULL,0,&save.settings,0,scratch)==NM_SAVE_OK;
    if(!sram_ok) return 0;
    if(eeprom_ok) return 1;

    /* If another title owns the EEPROM game block, SRAM is our safe fallback.
     * If our own block exists but still contains old settings, do not leave
     * Game Test: the main game would correctly prefer that stale EEPROM copy. */
    nm_settings_t persisted;
    if(nm_naomi_eeprom_load_settings(&persisted) &&
       memcmp(&persisted,&save.settings,sizeof(persisted))!=0)
        return 0;
    return 1;
}

static void change_assignment(unsigned item) {
    switch(item) {
    case ASSIGN_DIFFICULTY:
        save.settings.difficulty=(uint8_t)((save.settings.difficulty+1u)%NM_DIFFICULTY_COUNT);break;
    case ASSIGN_ADVERTISE:
        save.settings.advertise_sound=(uint8_t)((save.settings.advertise_sound+1u)%NM_ADVERTISE_COUNT);break;
    case ASSIGN_COIN_MODE:
        save.settings.coin_mode=(uint8_t)((save.settings.coin_mode+1u)%NM_COIN_MODE_COUNT);break;
    case ASSIGN_CONTROL:
        save.settings.control_scheme=(uint8_t)((save.settings.control_scheme+1u)%NM_CONTROL_COUNT);break;
    case ASSIGN_COUNTRY:
        save.settings.country=(uint8_t)((save.settings.country+1u)%NM_COUNTRY_COUNT);break;
    case ASSIGN_CABINET:
        save.settings.cabinet=(uint8_t)((save.settings.cabinet+1u)%NM_CABINET_COUNT);break;
    default: break;
    }
    dirty=1;
}

static void render_menu(char lines[LINE_COUNT][LINE_BYTES],unsigned selected) {
    snprintf(lines[0],LINE_BYTES,"DAYTONA USA GAME TEST MODE");
    snprintf(lines[1],LINE_BYTES,"SELECT: SERVICE BUTTON / ENTER: TEST");
    const char *items[MENU_COUNT]={"INPUT TEST","GAME ASSIGNMENTS","BOOKKEEPING","SOUND TEST","BACKUP DATA CLEAR","EXIT"};
    for(unsigned i=0;i<MENU_COUNT;++i)
        snprintf(lines[2+i],LINE_BYTES,"%s %s",selected==i?">":" ",items[i]);
    snprintf(lines[8],LINE_BYTES,"DIFFICULTY: %s",nm_settings_difficulty_name(save.settings.difficulty));
    snprintf(lines[9],LINE_BYTES,"CONTROL: %s",nm_settings_control_name(save.settings.control_scheme));
    snprintf(lines[10],LINE_BYTES,"COIN MODE: %s",nm_settings_coin_name(save.settings.coin_mode));
    snprintf(lines[11],LINE_BYTES,"%s",save_failed?"SAVE ERROR - CHECK SRAM":
        eeprom_warning?"EEPROM NOT UPDATED; SRAM SAVE IS ACTIVE":"EXIT RETURNS TO NAOMI SYSTEM MENU");
}

static void render_assign(char lines[LINE_COUNT][LINE_BYTES],unsigned selected) {
    snprintf(lines[0],LINE_BYTES,"GAME ASSIGNMENTS");
    snprintf(lines[1],LINE_BYTES,"SERVICE: SELECT / TEST: CHANGE");
    snprintf(lines[2],LINE_BYTES,"%s DIFFICULTY     %s",selected==0?">":" ",nm_settings_difficulty_name(save.settings.difficulty));
    snprintf(lines[3],LINE_BYTES,"%s ADVERTISE SOUND %s",selected==1?">":" ",nm_settings_advertise_name(save.settings.advertise_sound));
    snprintf(lines[4],LINE_BYTES,"%s COIN MODE       %s",selected==2?">":" ",nm_settings_coin_name(save.settings.coin_mode));
    snprintf(lines[5],LINE_BYTES,"%s CONTROL SCHEME  %s",selected==3?">":" ",nm_settings_control_name(save.settings.control_scheme));
    snprintf(lines[6],LINE_BYTES,"%s COUNTRY         %s",selected==4?">":" ",nm_settings_country_name(save.settings.country));
    snprintf(lines[7],LINE_BYTES,"%s CABINET         %s",selected==5?">":" ",nm_settings_cabinet_name(save.settings.cabinet));
    snprintf(lines[8],LINE_BYTES,"%s EXIT",selected==6?">":" ");
    snprintf(lines[9],LINE_BYTES,"CHANGES ARE SAVED WHEN YOU EXIT");
    snprintf(lines[10],LINE_BYTES," ");
    snprintf(lines[11],LINE_BYTES,"%s",save_failed?"SRAM SAVE ERROR":
        eeprom_warning?"EEPROM BLOCK IN USE; SETTINGS SAVED TO SRAM":" ");
}

static void render_input(char lines[LINE_COUNT][LINE_BYTES],mie_state_t *s) {
    snprintf(lines[0],LINE_BYTES,"INPUT TEST");
    if(!s) {
        snprintf(lines[2],LINE_BYTES,"MIE/JVS DEVICE NOT FOUND");
        snprintf(lines[11],LINE_BYTES,"PRESS TEST TO RETURN");
        return;
    }
    snprintf(lines[2],LINE_BYTES,"P1 SWITCHES: %04X",s->jvs.p1.raw);
    snprintf(lines[3],LINE_BYTES,"P2 SWITCHES: %04X",s->jvs.p2.raw);
    snprintf(lines[4],LINE_BYTES,"SYSTEM: %02X PANEL: %02X",s->jvs.system.raw,s->jvs.panel.psw.raw);
    snprintf(lines[5],LINE_BYTES,"WHEEL/PEDALS: %u %u %u",s->jvs.analog[0],s->jvs.analog[1],s->jvs.analog[2]);
    snprintf(lines[6],LINE_BYTES,"ANALOG 3-7: %u %u %u %u %u",s->jvs.analog[3],s->jvs.analog[4],s->jvs.analog[5],s->jvs.analog[6],s->jvs.analog[7]);
    snprintf(lines[7],LINE_BYTES,"P1 L/R: %u/%u  SW16/SW2: %u/%u",
        s->jvs.p1.left,s->jvs.p1.right,s->jvs.p1.sw16,s->jvs.p1.sw2);
    snprintf(lines[8],LINE_BYTES,"P1 START: %u SERVICE: %u",s->jvs.p1.start,s->jvs.p1.service);
    snprintf(lines[9],LINE_BYTES,"DIGITAL: L/R STEER, SW16/SW2 PEDALS");
    snprintf(lines[10],LINE_BYTES,"COIN1/2: %u/%u PULSE:%02X FAULT:%02X",
        (unsigned)s->jvs.coin[0].count,(unsigned)s->jvs.coin[1].count,
        (unsigned)s->jvs.coin_pulse,(unsigned)s->jvs.coin_fault);
    snprintf(lines[11],LINE_BYTES,"PRESS TEST TO RETURN");
}

static void render_bookkeeping(char lines[LINE_COUNT][LINE_BYTES]) {
    static const char *courses[3]={"BEGINNER","EXPERT","ADVANCED"};
    snprintf(lines[0],LINE_BYTES,"BOOKKEEPING - BEST RECORDS");
    for(unsigned i=0;i<3;++i) {
        const nm_race_record_t *r=&save.records[i][0];
        snprintf(lines[2+i],LINE_BYTES,"%s  BEST TIME: %lu",courses[i],(unsigned long)r->time);
    }
    snprintf(lines[7],LINE_BYTES,"TIMES ARE STORED IN 1/64 SECOND TICKS");
    snprintf(lines[11],LINE_BYTES,"PRESS TEST TO RETURN");
}

static const uint32_t bgm_commands[14] = {
    0xae1001,0xae1002,0xae1003,0xae1004,0xae1005,0xae1006,0xae1007,
    0xae1008,0xae1009,0xae100a,0xae100b,0xae100c,0xae100d,0xae100e
};
static const char *bgm_names[14] = {
    "START YOUR ENGINES","GAME OVER","BITCHING","LET'S GO AWAY DAYTONA",
    "GO TO THE VICTORY LANE","BREAK DOWN","THE KING OF SPEED","LET'S GO AWAY ADV",
    "SKY HIGH","CHECK IT UP","DREAM OF THE DAYTONA","ROLLING STARTED","CAN YOU FLY","DRY WAY"
};

static void render_sound(char lines[LINE_COUNT][LINE_BYTES],unsigned selected) {
    snprintf(lines[0],LINE_BYTES,"SOUND TEST");
    snprintf(lines[1],LINE_BYTES,"SERVICE: SELECT / TEST: PLAY");
    snprintf(lines[2],LINE_BYTES,"%s BGM TEST",selected==SOUND_BGM?">":" ");
    snprintf(lines[3],LINE_BYTES,"%s SE TEST",selected==SOUND_SE?">":" ");
    snprintf(lines[4],LINE_BYTES,"%s VOICE TEST",selected==SOUND_VOICE?">":" ");
    snprintf(lines[5],LINE_BYTES,"%s STOP ALL",selected==SOUND_STOP?">":" ");
    snprintf(lines[6],LINE_BYTES,"%s EXIT",selected==SOUND_EXIT?">":" ");
    snprintf(lines[8],LINE_BYTES,"%s",sound_ready?"AICAFLOW READY":"AUDIO / ROMFS UNAVAILABLE");
    snprintf(lines[11],LINE_BYTES,"TEST: ENTER");
}

static void render_bgm(char lines[LINE_COUNT][LINE_BYTES],unsigned selected) {
    snprintf(lines[0],LINE_BYTES,"BGM TEST");
    snprintf(lines[1],LINE_BYTES,"SERVICE: NEXT / TEST: PLAY");
    if(selected<14) {
        snprintf(lines[3],LINE_BYTES,"> %02u %s",selected+1,bgm_names[selected]);
        snprintf(lines[5],LINE_BYTES,"TRACK COMMAND: %06lX",(unsigned long)bgm_commands[selected]);
    } else if(selected==14) snprintf(lines[3],LINE_BYTES,"> STOP MUSIC");
    else snprintf(lines[3],LINE_BYTES,"> EXIT");
    snprintf(lines[11],LINE_BYTES,"TEST+SERVICE: BACK");
}

static void render_se(char lines[LINE_COUNT][LINE_BYTES],unsigned selected) {
    snprintf(lines[0],LINE_BYTES,"SOUND EFFECT TEST");
    snprintf(lines[1],LINE_BYTES,"SERVICE: NEXT / TEST: PLAY");
    if(selected<45) snprintf(lines[3],LINE_BYTES,"> SOUND EFFECT %02u",selected+1);
    else if(selected==45) snprintf(lines[3],LINE_BYTES,"> STOP EFFECTS");
    else snprintf(lines[3],LINE_BYTES,"> EXIT");
    snprintf(lines[5],LINE_BYTES,"COMMAND: 9E%02X7F",selected<45?selected+1:0);
    snprintf(lines[11],LINE_BYTES,"TEST: PLAY / SELECT");
}

static const uint32_t voice_commands[50] = {
    0xae2100,0xae2101,0xae2105,0xae2106,0xae2107,0xae2108,0xae210f,0xae2111,
    0xae2114,0xae2115,0xae2116,0xae2117,0xae211a,
    0xae2120,0xae2121,0xae2122,0xae2123,0xae2124,0xae2125,0xae2126,0xae2127,
    0xae2128,0xae2129,0xae212a,0xae212b,0xae212c,0xae212d,0xae212e,0xae212f,0xae2130,0xae2131,
    0xae2133,0xae2134,0xae2135,0xae2136,0xae2137,0xae2138,
    0xae2163,0xae2164,0xae2165,0xae2166,0xae2167,0xae2168,0xae2169,0xae216a,0xae216b,0xae216c,0xae216d,
    0xae2170,0xae2171
};

static void render_voice(char lines[LINE_COUNT][LINE_BYTES],unsigned selected) {
    snprintf(lines[0],LINE_BYTES,"VOICE TEST");
    snprintf(lines[1],LINE_BYTES,"SERVICE: NEXT / TEST: PLAY");
    snprintf(lines[3],LINE_BYTES,"> ANNOUNCER PHRASE %02u",selected+1);
    snprintf(lines[5],LINE_BYTES,"COMMAND: %06lX",(unsigned long)voice_commands[selected]);
    snprintf(lines[11],LINE_BYTES,"TEST+SERVICE: BACK");
}

static void render_clear(char lines[LINE_COUNT][LINE_BYTES],unsigned selected) {
    snprintf(lines[0],LINE_BYTES,"BACKUP DATA CLEAR");
    snprintf(lines[2],LINE_BYTES,"%s YES",selected==0?">":" ");
    snprintf(lines[3],LINE_BYTES,"%s NO",selected==1?">":" ");
    snprintf(lines[5],LINE_BYTES,"THIS CLEARS RECORDS, NOT SETTINGS");
    snprintf(lines[7],LINE_BYTES,"PRESS TEST TO CONFIRM");
    snprintf(lines[11],LINE_BYTES,"SERVICE: SELECT");
}

int main(int argc,char **argv) {
    (void)argc;(void)argv;
    char lines[LINE_COUNT][LINE_BYTES]={{0}};
    nm_save_defaults(&save,nm_race_record_seeds);
    const nm_save_result_t loaded=nm_save_store_load(nm_naomi_save_device(),&save);
    printf("Daytona Game Test: SRAM load %s\n",nm_save_result_name(loaded));
    /* The Naomi EEPROM is authoritative for operator settings; the SRAM file carries records. */
    if(!nm_naomi_eeprom_load_settings(&save.settings))
        printf("Daytona Game Test: EEPROM settings unavailable; retaining SRAM/default settings\n");

    if(naomi_romfs_init_default()==0) {
        sound_ready=nm_sound_init("/naomi/");
        printf("Daytona Game Test: sound %s\n",sound_ready?"ready":"unavailable");
    } else printf("Daytona Game Test: ROMFS mount failed; sound test unavailable\n");

    pvr_init_defaults();
    pvr_set_bg_color(0.0f,0.0f,0.0f);
    font_tex=pvr_mem_malloc(256*256*2);
    if(!font_tex) for(;;) thd_sleep(1000);
    font_init();
    maple_wait_scan();
    mie_state_t *initial=mie_status();
    /* BIOS may launch GAME TEST while the cabinet TEST switch is still held.
     * Treat that held state as the initial level, not as a fresh menu press. */
    int previous_service=initial?!!initial->jvs.p1.service:0;
    int previous_test=initial?!!(initial->jvs.system.test||initial->jvs.panel.psw.test):0;
    unsigned page=PAGE_MENU,menu_selected=0,assign_selected=0,clear_selected=1;

    for(;;) {
        mie_state_t *s=mie_status();
        const int service=s?!!s->jvs.p1.service:0;
        const int test=s?!!(s->jvs.system.test||s->jvs.panel.psw.test):0;
        const int service_edge=service&&!previous_service;
        const int test_edge=test&&!previous_test;
        previous_service=service;previous_test=test;

        memset(lines,0,sizeof(lines));
        if(page==PAGE_MENU) {
            if(service_edge) menu_selected=(menu_selected+1u)%MENU_COUNT;
            if(test_edge) {
                switch(menu_selected) {
                case MENU_INPUT: page=PAGE_INPUT;break;
                case MENU_ASSIGN: page=PAGE_ASSIGN;assign_selected=0;break;
                case MENU_BOOKKEEPING: page=PAGE_BOOKKEEPING;break;
                case MENU_SOUND: page=PAGE_SOUND;sound_selected=0;break;
                case MENU_CLEAR: page=PAGE_CLEAR;clear_selected=1;break;
                case MENU_EXIT:
                    if(dirty) save_failed=!save_now();
                    if(!save_failed) { if(sound_ready) nm_sound_shutdown(); naomi_romfs_shutdown(); exit_to_system_menu(); }
                    break;
                }
            }
            render_menu(lines,menu_selected);
        } else if(page==PAGE_INPUT) {
            if(test_edge) page=PAGE_MENU;
            render_input(lines,s);
        } else if(page==PAGE_ASSIGN) {
            if(service_edge) assign_selected=(assign_selected+1u)%ASSIGN_COUNT;
            if(test_edge) {
                if(assign_selected==ASSIGN_EXIT) {
                    if(dirty) save_failed=!save_now();
                    if(!save_failed) dirty=0;
                    page=PAGE_MENU;
                } else change_assignment(assign_selected);
            }
            render_assign(lines,assign_selected);
        } else if(page==PAGE_BOOKKEEPING) {
            if(test_edge) page=PAGE_MENU;
            render_bookkeeping(lines);
        } else if(page==PAGE_SOUND) {
            if(service_edge) sound_selected=(sound_selected+1u)%SOUND_COUNT;
            if(test_edge) {
                if(sound_selected==SOUND_BGM) { page=PAGE_BGM;assign_selected=0; }
                else if(sound_selected==SOUND_SE) { page=PAGE_SE;assign_selected=0; }
                else if(sound_selected==SOUND_VOICE) { page=PAGE_VOICE;assign_selected=0; }
                else if(sound_selected==SOUND_STOP && sound_ready) {
                    nm_sound_command(0xbe507e);
                    nm_sound_command(0xbe507d);
                } else if(sound_selected==SOUND_EXIT) page=PAGE_MENU;
            }
            render_sound(lines,sound_selected);
        } else if(page==PAGE_BGM) {
            if(service && test) page=PAGE_SOUND;
            else {
            if(service_edge) assign_selected=(assign_selected+1u)%16u;
            if(test_edge) {
                if(assign_selected<14 && sound_ready) {
                    nm_sound_command(bgm_commands[assign_selected]);
                    nm_sound_settle();
                } else if(assign_selected==14 && sound_ready) nm_sound_command(0xbe507e);
                else if(assign_selected==15) page=PAGE_SOUND;
            }
            }
            render_bgm(lines,assign_selected);
        } else if(page==PAGE_SE) {
            if(service && test) page=PAGE_SOUND;
            else {
            if(service_edge) assign_selected=(assign_selected+1u)%47u;
            if(test_edge) {
                if(assign_selected<45 && sound_ready)
                    nm_sound_command(0x009e007fu | ((uint32_t)(assign_selected+1u)<<8));
                else if(assign_selected==45 && sound_ready) nm_sound_command(0xbe507d);
                else if(assign_selected==46) page=PAGE_SOUND;
            }
            }
            render_se(lines,assign_selected);
        } else if(page==PAGE_VOICE) {
            if(service && test) page=PAGE_SOUND;
            else {
                if(service_edge) assign_selected=(assign_selected+1u)%50u;
                if(test_edge && sound_ready) nm_sound_command(voice_commands[assign_selected]);
            }
            render_voice(lines,assign_selected);
        } else if(page==PAGE_CLEAR) {
            if(service_edge) clear_selected^=1u;
            if(test_edge) {
                if(clear_selected==0) {
                    const nm_settings_t settings=save.settings;
                    nm_save_defaults(&save,nm_race_record_seeds);
                    save.settings=settings;
                    dirty=1;
                    save_failed=!save_now();
                    if(!save_failed) dirty=0;
                }
                page=PAGE_MENU;
            }
            render_clear(lines,clear_selected);
        }
        if(sound_ready) nm_sound_update();
        draw_lines(lines);
        thd_sleep(50);
    }
    return 0;
}
