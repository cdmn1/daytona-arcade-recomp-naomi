#ifndef DAYTONA_NAOMI_SAVE_H
#define DAYTONA_NAOMI_SAVE_H

#include "save_store.h"

/* Battery-backed Naomi SRAM save area reserved for this native port. */
const nm_save_device_t *nm_naomi_save_device(void);

#endif
