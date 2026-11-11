#ifndef DAYTONA_NAOMI_ROMFS_H
#define DAYTONA_NAOMI_ROMFS_H

/*
 * Mount the read-only ROMFS appended after a Naomi executable segment.
 * Files are exposed below /naomi, for example /naomi/music/t01.adx.
 *
 * Call after KOS initialization and before starting the game or loading assets.
 */
int naomi_romfs_init_default(void);
void naomi_romfs_shutdown(void);

#endif
