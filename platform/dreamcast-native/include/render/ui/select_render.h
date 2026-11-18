#ifndef DAYTONA_NATIVE_RENDER_SELECT_RENDER_H
#define DAYTONA_NATIVE_RENDER_SELECT_RENDER_H
#include "select.h"

/* The country-specific course/transmission select and GENTLEMEN screens (tools/prepare_select.py:
 * ROOT/select/select.dsl, packaged as ROOT/select-usa|jpn|export/select/; only the selected country is loaded).
 * The pose comes from select.h and frame counter 5010b0 (the panels' and frame's blink). */
int nm_select_render_load(const char *root, unsigned country);
int nm_select_render_enqueue(const nm_select_t *select, uint32_t counter);
int nm_select_render_release(void);

#endif
