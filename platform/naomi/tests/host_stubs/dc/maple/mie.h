#ifndef NAOMI_HOST_STUB_DC_MAPLE_MIE_H
#define NAOMI_HOST_STUB_DC_MAPLE_MIE_H
#include <stdint.h>
#include <stdbool.h>
#include <dc/maple/controller.h>
#define MIE_PORT0_JVS 1
#define MIE_PORT0_MAPLE 2
#define MIE_EEPROM_SIZE 128
#define MIE_JVS_SW10_BIT (1u << 1)
#define MIE_JVS_UP_BIT (1u << 13)
#define MIE_JVS_RIGHT_BIT (1u << 10)
#define MIE_JVS_DOWN_BIT (1u << 12)
#define MIE_JVS_LEFT_BIT (1u << 11)
typedef struct mie_jvs_inputs {
    uint16_t wheel;
    uint16_t accel;
    uint16_t brake;
    struct { uint16_t raw; } p1, p2;
    struct {
        struct { uint8_t raw; } dip;
        struct { uint8_t raw; } psw;
    } panel;
    struct { uint16_t count; uint16_t raw; } coin[2];
    uint8_t coin_pulse;
    uint8_t coin_fault;
    struct { uint8_t raw; } system;
} mie_jvs_inputs_t;
typedef struct mie_state {
    cont_state_t cont;
    mie_jvs_inputs_t jvs;
} mie_state_t;
int mie_port0_mode(void);
bool mie_get_eeprom(void *dst);
bool mie_set_eeprom(uint8_t *eeprom);
#endif
