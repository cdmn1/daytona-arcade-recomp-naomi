#ifndef NAOMI_HOST_STUB_DC_MAPLE_CONTROLLER_H
#define NAOMI_HOST_STUB_DC_MAPLE_CONTROLLER_H
#include <stdint.h>
typedef struct cont_state {
    uint32_t buttons;
    int ltrig;
    int rtrig;
    int joyx;
    int joyy;
} cont_state_t;
#endif
