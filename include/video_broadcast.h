//Copyright 2016 <>< Charles Lohr, See LICENSE file.
// COPYRIGHT 2022 Paul Schlarmann

#ifndef _VIDEO_BROADCAST_TEST
#define _VIDEO_BROADCAST_TEST

/*
 * Call video_init() to start RF channel-3 output on GP21 (Pico physical pin
 * 27). PIO serializes the precomputed RF waveform while DMA supplies video
 * line data.
 */

#ifdef __cplusplus
extern "C" {
#endif
//Stuff that should be for the header:


#include <stdint.h>
//Framebuffer width/height
#define FBW 232 //Must be divisible by 8.  These are actually "double-pixels" used for double-resolution monochrome width.
#define FBW2 (FBW/2) //Actual width in true pixels.
#ifdef PAL
#define FBH 264
#else
#define FBH 220
#endif

#define DMABUFFERDEPTH 2

extern volatile int gframe; //Current frame #
extern uint16_t framebuffer[((FBW2/4)*(FBH))*2]; // /4 = 4 pixels per word (*2 = double buffer)
extern int8_t jam_color; //Used to test frequency out


void video_init(void);
#ifdef __cplusplus
}
#endif
#endif
