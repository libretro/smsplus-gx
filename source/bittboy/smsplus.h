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

typedef struct {
  int8_t* current_pos;
  int8_t* buffer;
  int32_t current_emulated_samples;
} t_sdl_sound;

typedef struct {
  uint32_t ticks_starting;
  SDL_sem* sem_sync;
} t_sdl_sync;

void smsp_state(uint8_t slot, uint8_t mode);

#endif
