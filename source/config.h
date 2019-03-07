
#ifndef CONFIG_H__
#define CONFIG_H__


typedef struct {
  char game_name[0x100];
  int32_t frameskip;
  int32_t fullscreen;
  int32_t filter;
  int32_t fullspeed;
  int32_t nosound;
  int32_t joystick;
  int32_t sndrate;
  int32_t country;
  int32_t console;
  int32_t fm;
  int32_t overscan;
  int32_t ntsc;
  uint32_t use_bios;
  int32_t spritelimit;
  int32_t extra_gg;
  int32_t tms_pal;
} t_config;


extern t_config option;

#endif
