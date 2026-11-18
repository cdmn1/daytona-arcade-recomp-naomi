#ifndef DAYTONA_NATIVE_SETTINGS_H
#define DAYTONA_NATIVE_SETTINGS_H
#include <stdint.h>

/* Operator settings (original EEPROM fields where present; see tools/settings_eeprom.py).
 * Country and cabinet retain the original enumerations: COUNTRY +1b (USA/JPN/EXPORT, factory JPN),
 * CABINET +1a (DELUXE/UPLIGHT, factory DELUXE). Naomi-only controls and coin mode live in this title's
 * separate DUSA MIE EEPROM block, not at the Model 2 offsets. */
enum { NM_DIFFICULTY_EASY, NM_DIFFICULTY_NORMAL, NM_DIFFICULTY_HARD, NM_DIFFICULTY_HARDEST, NM_DIFFICULTY_COUNT };
enum { NM_ADVERTISE_ON, NM_ADVERTISE_OFF, NM_ADVERTISE_COUNT };
enum { NM_CONTROL_ANALOG, NM_CONTROL_DIGITAL, NM_CONTROL_COUNT };
enum { NM_COIN_ONE_CREDIT, NM_COIN_FREE_PLAY, NM_COIN_MODE_COUNT };
enum { NM_RESET_BUTTON_OFF, NM_RESET_BUTTON_ON, NM_RESET_BUTTON_COUNT };
enum { NM_COUNTRY_USA, NM_COUNTRY_JPN, NM_COUNTRY_EXPORT, NM_COUNTRY_COUNT };
enum { NM_CABINET_DELUXE, NM_CABINET_UPLIGHT, NM_CABINET_COUNT };

typedef struct {
    uint8_t difficulty, advertise_sound;
    uint8_t control_scheme, reset_with_p1_d, coin_mode;
    uint8_t cabinet, country;
} nm_settings_t;

static inline nm_settings_t nm_settings_factory(void) {
    return (nm_settings_t){.difficulty=NM_DIFFICULTY_NORMAL,.advertise_sound=NM_ADVERTISE_ON,
        .control_scheme=NM_CONTROL_ANALOG,.reset_with_p1_d=NM_RESET_BUTTON_ON,.coin_mode=NM_COIN_ONE_CREDIT,
        .cabinet=NM_CABINET_DELUXE,.country=NM_COUNTRY_JPN};
}

static inline int nm_settings_valid(const nm_settings_t *s) {
    return s && s->difficulty<NM_DIFFICULTY_COUNT && s->advertise_sound<NM_ADVERTISE_COUNT &&
        s->control_scheme<NM_CONTROL_COUNT && s->reset_with_p1_d<NM_RESET_BUTTON_COUNT && s->coin_mode<NM_COIN_MODE_COUNT &&
        s->cabinet<NM_CABINET_COUNT && s->country<NM_COUNTRY_COUNT;
}

static inline const char *nm_settings_control_name(unsigned scheme) {
    return scheme==NM_CONTROL_ANALOG ? "ANALOG" : scheme==NM_CONTROL_DIGITAL ? "DIGITAL" : "?";
}
static inline const char *nm_settings_coin_name(unsigned mode) {
    return mode==NM_COIN_FREE_PLAY ? "FREE PLAY" : mode==NM_COIN_ONE_CREDIT ? "1 COIN = 1 CREDIT" : "?";
}
static inline const char *nm_settings_reset_name(unsigned enabled) {
    return enabled==NM_RESET_BUTTON_ON ? "ON" : enabled==NM_RESET_BUTTON_OFF ? "OFF" : "?";
}

/* The names of the original's test mode (23c710, 23c6e0), trailing blanks dropped. */
static inline const char *nm_settings_difficulty_name(unsigned difficulty) {
    static const char *const names[NM_DIFFICULTY_COUNT]={"EASY","NORMAL","HARD","HARDEST"};
    return difficulty<NM_DIFFICULTY_COUNT ? names[difficulty] : "?";
}
static inline const char *nm_settings_advertise_name(unsigned advertise) {
    return advertise==NM_ADVERTISE_ON ? "ON" : advertise==NM_ADVERTISE_OFF ? "OFF" : "?";
}
static inline const char *nm_settings_country_name(unsigned country) {
    static const char *const names[NM_COUNTRY_COUNT]={"USA","JPN","EXPORT"};
    return country<NM_COUNTRY_COUNT ? names[country] : "?";
}
static inline const char *nm_settings_cabinet_name(unsigned cabinet) {
    return cabinet==NM_CABINET_DELUXE ? "DELUXE" : cabinet==NM_CABINET_UPLIGHT ? "UPLIGHT" : "?";
}

/* A difficulty by its name, either case (tools: trace_race_native --difficulty); -1 when none. */
static inline int nm_settings_difficulty_parse(const char *name) {
    for(unsigned d=0;name && d<NM_DIFFICULTY_COUNT;++d) {
        const char *a=name,*b=nm_settings_difficulty_name(d);
        while(*a && *b && (*a|0x20)==(*b|0x20)) { ++a;++b; }
        if(!*a && !*b) return (int)d;
    }
    return -1;
}
#endif
