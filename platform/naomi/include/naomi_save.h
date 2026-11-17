#ifndef DAYTONA_NAOMI_SAVE_H
#define DAYTONA_NAOMI_SAVE_H

#include "save_store.h"

/* Battery-backed Naomi SRAM save area reserved for this native port. */
const nm_save_device_t *nm_naomi_save_device(void);

/* Naomi MIE game-settings block (distinct from the Model 2 XMODEM image). */
int nm_naomi_eeprom_load_settings(nm_settings_t *settings);
int nm_naomi_eeprom_store_settings(const nm_settings_t *settings);

#endif
