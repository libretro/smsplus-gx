
#ifndef __CONFIG_H__
#define __CONFIG_H__


typedef struct {
  char game_name[0x100];
  int frameskip;
  int fullscreen;
  int filter;
  int fullspeed;
  int nosound;
  int joystick;
  int sndrate;
  int country;
  int console;
  int fm;
  int overscan;
  int ntsc;
  int use_bios;
  int spritelimit;
  int extra_gg;
  int tms_pal;
} t_config;


extern t_config option;


#endif
