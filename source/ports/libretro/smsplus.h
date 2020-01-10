#ifndef SMSPLUS_H
#define SMSPLUS_H

//#include <SDL/SDL.h>

#define HOST_WIDTH_RESOLUTION 640
#define HOST_HEIGHT_RESOLUTION 480

#define VIDEO_WIDTH_SMS 256
#define VIDEO_HEIGHT_SMS 192
#define VIDEO_WIDTH_GG 160
#define VIDEO_HEIGHT_GG 144

#define LOCK_VIDEO
#define UNLOCK_VIDEO

typedef struct {
	char gamename[256];
	char sramdir[256];
	char sramfile[256];
	char stdir[256];
	char scrdir[256];
	char biosdir[256];
} gamedata_t;

void smsp_state(uint8_t slot_number, uint8_t mode);

#define SOUND_FREQUENCY 44100
#define SOUND_SAMPLES_SIZE 1024

#endif
