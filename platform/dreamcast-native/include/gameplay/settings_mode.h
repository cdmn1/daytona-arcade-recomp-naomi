#ifndef DAYTONA_NATIVE_GAMEPLAY_SETTINGS_MODE_H
#define DAYTONA_NATIVE_GAMEPLAY_SETTINGS_MODE_H

/* The settings screen until a real options menu exists (code/gameplay/settings_mode.c): what the
 * original's test mode GAME ASSIGNMENTS offers a single-player game (include/settings.h), control scheme, reset option, records' reset
 * (BACKUP DATA CLEAR without the bookkeeping) and save status. From the title: the cabinet TEST switch on Naomi, or both triggers held and START as a fallback.
 * Up/down select, left/right (or A) change, START or EXIT applies settings and returns to the title, B returns
 * without changing anything. */
int nm_settings_mode_start(void);

#endif
