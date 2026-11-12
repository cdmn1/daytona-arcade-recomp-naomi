#ifndef NAOMI_HOST_STUB_VIDEO_H
#define NAOMI_HOST_STUB_VIDEO_H
#include <stdint.h>
typedef struct { int width; int height; } naomi_host_vid_mode_t;
extern naomi_host_vid_mode_t *vid_mode;
extern uint16_t *vram_s;
#endif
