#ifndef _SMSPLUS_H
#define _SMSPLUS_H

#define VIDEO_WIDTH_SMS 256
#define VIDEO_HEIGHT_SMS 192
#define VIDEO_WIDTH_GG 160
#define VIDEO_HEIGHT_GG 144

typedef struct {
	int8_t gamename[256];
	int8_t sramdir[256];
	int8_t sramfile[516];
	int8_t stdir[256];
	int8_t scrdir[256];
	int8_t biosdir[256];
} gamedata_t;

void smsp_state(uint8_t slot, uint8_t mode);

#define SOUND_FREQUENCY 48000
#define SOUND_SAMPLES_SIZE 2048

#endif
