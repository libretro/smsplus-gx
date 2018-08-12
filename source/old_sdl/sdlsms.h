
#ifndef __SDLSMS_H__
#define __SDLSMS_H__

#include "shared.h"



#define SMSSDL_CORE_VER  "1"
#define SMSSDL_RELEASE   "7"
#define SMSSDL_TITLE     "SMS Plus/SDL v" SMSSDL_CORE_VER "R" SMSSDL_RELEASE

#define SMS_SCREEN_WIDTH  256
#define SMS_SCREEN_HEIGHT 192
#define GG_SCREEN_WIDTH   160
#define GG_SCREEN_HEIGHT  144

#define FILTER_MARGIN_HEIGHT 3

#define SOUND_FREQUENCY    48000
#define SOUND_SAMPLES_SIZE  2048

#define MACHINE_FPS 60


typedef struct {
  int up,down,left,right;
  int b1,b2;
  int start;
} t_paddle;

typedef void (*t_filterfunc)(Uint8*,Uint32,Uint8*,Uint32,int,int);

typedef struct {
  SDL_Surface* surf_screen;
  SDL_Surface* surf_bitmap;
  SDL_Surface* surf_filter;
  int width, height;
  Uint32 frames_rendered;
  int current_filter;
  int frame_skip;
  int current_screenshot;
  int bitmap_offset;
} t_sdl_video;

typedef struct {
  char* current_pos;
  char* buffer;
  int current_emulated_samples;
} t_sdl_sound;

typedef struct {
  int state_slot;
  t_paddle pad[2];
} t_sdl_controls;

typedef struct {
  SDL_Joystick* joy;
  int commit_range;
  int xstatus, ystatus;
  int number;
  int map_b1, map_b2, map_start;
} t_sdl_joystick;

typedef struct {
  Uint32 ticks_starting;
  SDL_sem* sem_sync;
} t_sdl_sync;


extern int sdlsms_init();
extern void sdlsms_emulate();
extern void sdlsms_shutdown();



#endif
