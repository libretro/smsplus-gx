
/*
    Copyright (c) 2002, 2003 Gregory Montoir

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "shared.h"
#include "saves.h"
#include "sdlsms.h"
#include "font.h"
#include "text_gui.h"

static const char *rom_filename;
t_sdl_video sdl_video;
static t_sdl_sound sdl_sound;
static t_sdl_controls sdl_controls;
static t_sdl_joystick sdl_joystick;
static t_sdl_sync sdl_sync;

static uint32_t time_state = 0;
static int8_t buf[24];
/* video */

SDL_Surface* data_screen;

static void sdlsms_video_blit_center(SDL_Surface* screen, SDL_Surface* buffer)
{
	SDL_LockSurface(screen);
	bitmap.data = (unsigned char*)screen->pixels + (4 * (3072 - 16));
	if (time_state > 120)
	{
		time_state = 0;
		memset(screen->pixels, 0, (320*24));
	}
	else if (time_state > 0)
	{
		if (time_state == 1) memset(screen->pixels, 0, (320*24));
		gfx_font_print(screen, 4, 4, font, buf);
		time_state++;
	}
	SDL_Flip(screen);
	SDL_UnlockSurface(screen);
	/*bitmap.data = (unsigned char*)buffer->pixels + (4 * (3072 - 16));
	if (time_state > 120)
	{
		time_state = 0;
		memset(buffer->pixels, 0, (320*24));
	}
	else if (time_state > 0)
	{
		if (time_state == 1) memset(buffer->pixels, 0, (320*24));
		gfx_font_print(buffer->pixels, 4, 4, font, buf);
		time_state++;
	}*/
	//SDL_SoftStretch(buffer, NULL, screen, NULL);
	//SDL_Flip(screen);
}

static void sdlsms_video_take_screenshot()
{
	int status;
  char ssname[0x100];

  strcpy(ssname, rom_filename);
  sprintf(strrchr(ssname, '.'), "-%03d.bmp", sdl_video.current_screenshot);
  ++sdl_video.current_screenshot;
  SDL_LockSurface(sdl_video.surf_screen);
  status = SDL_SaveBMP(sdl_video.surf_screen, ssname);
  SDL_UnlockSurface(sdl_video.surf_screen);
  if(status == 0)
    printf("[INFO] Screenshot written to '%s'.\n", ssname);
}

static int sdlsms_video_init(int frameskip, int fullscreen, int filter)
{
  int screen_width, screen_height;
  Uint32 vidflags = SDL_HWSURFACE;

  screen_width  = (IS_GG) ? GG_SCREEN_WIDTH  : SMS_SCREEN_WIDTH;
  screen_height = (IS_GG) ? GG_SCREEN_HEIGHT : SMS_SCREEN_HEIGHT;

  sdl_video.current_filter = -1;

  if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    printf("ERROR: %s.\n", SDL_GetError());
    return 0;
  }


  sdl_video.surf_screen = SDL_SetVideoMode(320, 240, 16, vidflags);
  if(!sdl_video.surf_screen) {
    printf("ERROR: can't set video mode (%dx%d): %s.\n", screen_width, screen_height, SDL_GetError());
    return 0;
  }

  if(fullscreen) {
    SDL_ShowCursor(SDL_DISABLE);
  }

  SDL_WM_SetCaption(SMSSDL_TITLE, NULL);
  return 1;
}

static void sdlsms_video_finish_update()
{
	sdlsms_video_blit_center(sdl_video.surf_screen, data_screen);
}

static void sdlsms_video_update()
{
	sms_frame(0);
    sdlsms_video_finish_update();
	++sdl_video.frames_rendered;
}

static void sdlsms_video_close()
{
  if(sdl_video.surf_screen) SDL_FreeSurface(sdl_video.surf_screen);
  printf("[INFO] Frames rendered = %lu.\n", sdl_video.frames_rendered);
}


/* sound */

static void sdlsms_sound_callback(void *userdata, Uint8 *stream, int len)
{
  if(sdl_sound.current_emulated_samples < len) {
    memset(stream, 0, len);
  }
  else {
    memcpy(stream, sdl_sound.buffer, len);
    /* loop to compensate desync */
    do {
      sdl_sound.current_emulated_samples -= len;
    } while(sdl_sound.current_emulated_samples > 2 * len);
    memcpy(sdl_sound.buffer,
           sdl_sound.current_pos - sdl_sound.current_emulated_samples,
           sdl_sound.current_emulated_samples);
    sdl_sound.current_pos = sdl_sound.buffer + sdl_sound.current_emulated_samples;
  }
}

static int sdlsms_sound_init()
{
  int n;
  SDL_AudioSpec as_desired, as_obtained;
  
  if(SDL_Init(SDL_INIT_AUDIO) < 0) {
    printf("ERROR: %s.\n", SDL_GetError());
    return 0;
  }

  as_desired.freq = SOUND_FREQUENCY;
  as_desired.format = AUDIO_S16LSB;
  as_desired.channels = 2;
  as_desired.samples = SOUND_SAMPLES_SIZE;
  as_desired.callback = sdlsms_sound_callback;

  if(SDL_OpenAudio(&as_desired, &as_obtained) == -1) {
    printf("ERROR: can't open audio: %s.\n", SDL_GetError());
    return 0;
  }

  if(as_desired.samples != as_obtained.samples) {
    printf("ERROR: soundcard driver does not accept specified samples size.\n");
    return 0;
  }

  sdl_sound.current_emulated_samples = 0;
  n = SOUND_SAMPLES_SIZE * 2 * sizeof(short) * 11;
  sdl_sound.buffer = (char*)malloc(n);
  if(!sdl_sound.buffer) {
    printf("ERROR: can't allocate memory for sound.\n");
    return 0;
  }
  memset(sdl_sound.buffer, 0, n);
  sdl_sound.current_pos = sdl_sound.buffer;
  return 1;
}

static void sdlsms_sound_update()
{
  int i;
  short* p;

  SDL_LockAudio();
  p = (short*)sdl_sound.current_pos;
  for(i = 0; i < snd.bufsize; ++i) {
      *p = snd.buffer[0][i];
      ++p;
      *p = snd.buffer[1][i];
      ++p;
  }
  sdl_sound.current_pos = (char*)p;
  sdl_sound.current_emulated_samples += snd.bufsize * 2 * sizeof(short);
  SDL_UnlockAudio();
}

static void sdlsms_sound_close()
{
  SDL_PauseAudio(1);
  SDL_CloseAudio();
  free(sdl_sound.buffer);
}


/* controls */

static void sdlsms_controls_init()
{
  sdl_controls.state_slot = 0;
  sdl_controls.pad[0].up = SDLK_UP;
  sdl_controls.pad[0].down = SDLK_DOWN;
  sdl_controls.pad[0].left = SDLK_LEFT;
  sdl_controls.pad[0].right = SDLK_RIGHT;
  sdl_controls.pad[0].b1 = SDLK_LCTRL;
  sdl_controls.pad[0].b2 = SDLK_LALT;
  sdl_controls.pad[0].start = SDLK_RETURN;
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

static int sdlsms_controls_update(SDLKey k, int p)
{
  if(!p) {
	  switch(k) {
	  case SDLK_ESCAPE:
		  return 0;
	  case SDLK_F1:
		  sdlsms_video_take_screenshot();
		  break;
	  case SDLK_TAB:
		  if(save_state(rom_filename, sdl_controls.state_slot))
  		  printf("[INFO] Saved state to slot #%d.\n", sdl_controls.state_slot);
  		  snprintf(buf, sizeof(buf), "Saved state slot %d", sdl_controls.state_slot);
  		  time_state = 1;
		  break;
	  case SDLK_BACKSPACE:
		  if(load_state(rom_filename, sdl_controls.state_slot))
		    printf("[INFO] Loaded state from slot #%d.\n", sdl_controls.state_slot);
  		  snprintf(buf, sizeof(buf), "Loaded state slot %d", sdl_controls.state_slot);
  		  time_state = 1;
		  break;
    case SDLK_SPACE:
		sdl_controls.state_slot++;
		if(sdl_controls.state_slot > 9)
		sdl_controls.state_slot = 0;
		snprintf(buf, sizeof(buf), "State slot : %d", sdl_controls.state_slot);
		time_state = 1;
		printf("[INFO] Slot changed to #%d.\n", sdl_controls.state_slot);
      break;
    }
  }
  if(k == sdl_controls.pad[0].start) {
    if(p) input.system |= (IS_GG) ? INPUT_START : INPUT_PAUSE;
    else  input.system &= (IS_GG) ? ~INPUT_START : ~INPUT_PAUSE;
  }
  else if(k == sdl_controls.pad[0].up) {
    if(p) input.pad[0] |= INPUT_UP;
    else  input.pad[0] &= ~INPUT_UP;
  }
  else if(k == sdl_controls.pad[0].down) {
    if(p) input.pad[0] |= INPUT_DOWN;
    else  input.pad[0] &= ~INPUT_DOWN;
  }
  else if(k == sdl_controls.pad[0].left) {
    if(p) input.pad[0] |= INPUT_LEFT;
    else  input.pad[0] &= ~INPUT_LEFT;
  }
  else if(k == sdl_controls.pad[0].right) {
    if(p) input.pad[0] |= INPUT_RIGHT;
    else  input.pad[0] &= ~INPUT_RIGHT;
  }
  else if(k == sdl_controls.pad[0].b1) {
    if(p) input.pad[0] |= INPUT_BUTTON1;
    else  input.pad[0] &= ~INPUT_BUTTON1;
  }
  else if(k == sdl_controls.pad[0].b2) {
    if(p) input.pad[0] |= INPUT_BUTTON2;
    else  input.pad[0] &= ~INPUT_BUTTON2;
  }
  /*else if(k == SDLK_TAB) {
    if(p) input.system |= INPUT_HARD_RESET;
    else  input.system &= ~INPUT_HARD_RESET;
  }*/
  return 1;
}



/* joystick */

static int smssdl_joystick_init()
{
  if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
    printf("ERROR: %s.\n", SDL_GetError());
    return 0;
  }
  else {
    sdl_joystick.commit_range = 3276;
    sdl_joystick.xstatus = 1;
    sdl_joystick.ystatus = 1;
    sdl_joystick.number = 0;
    sdl_joystick.map_b1 = 1;
    sdl_joystick.map_b2 = 2;
    sdl_joystick.map_start = 4;
    sdl_joystick.joy = SDL_JoystickOpen(sdl_joystick.number);
    if(sdl_joystick.joy != NULL) {
      printf("Joystick (%s) has %d axes and %d buttons.\n", SDL_JoystickName(sdl_joystick.number), SDL_JoystickNumAxes(sdl_joystick.joy), SDL_JoystickNumButtons(sdl_joystick.joy));
      printf("Using button mapping I=%d II=%d START=%d.\n", sdl_joystick.map_b1, sdl_joystick.map_b2, sdl_joystick.map_start);
      SDL_JoystickEventState(SDL_ENABLE);
      return 1;
    }
    else {
      printf("ERROR: Could not open joystick: %s.\n", SDL_GetError());
      return 0;
    }
  }
}


static void smssdl_joystick_update(SDL_Event event)
{
	int axisval;
  switch(event.type) {
	case SDL_JOYAXISMOTION:
		switch(event.jaxis.axis) {
		case 0:	/* X axis */
			axisval = event.jaxis.value;

			if(axisval > sdl_joystick.commit_range) {
				if(sdl_joystick.xstatus == 2)
					break;
				if(sdl_joystick.xstatus == 0) {
					input.pad[0] &= ~INPUT_LEFT;
				}
				input.pad[0] |= INPUT_RIGHT;
				sdl_joystick.xstatus = 2;
				break;
			}

			if(axisval < -sdl_joystick.commit_range) {
				if(sdl_joystick.xstatus == 0)
					break;
				if(sdl_joystick.xstatus == 2) {
				  input.pad[0] &= ~INPUT_RIGHT;
				}
				input.pad[0] |= INPUT_LEFT;
				sdl_joystick.xstatus = 0;
				break;
			}

			/* axis is centered */
			if(sdl_joystick.xstatus == 2) {
				input.pad[0] &= ~INPUT_RIGHT;
			}
			if(sdl_joystick.xstatus == 0) {
				input.pad[0] &= ~INPUT_LEFT;
			}
			sdl_joystick.xstatus = 1;
			break;

		case 1:	/* Y axis */
			axisval = event.jaxis.value;

			if(axisval > sdl_joystick.commit_range) {
				if(sdl_joystick.ystatus == 2)
					break;
				if(sdl_joystick.ystatus == 0) {
					input.pad[0] &= ~INPUT_UP;
				}
				input.pad[0] |= INPUT_DOWN;
				sdl_joystick.ystatus = 2;
				break;
			}
			
      if(axisval < -sdl_joystick.commit_range) {
				if(sdl_joystick.ystatus == 0)
					break;
				if(sdl_joystick.ystatus == 2) {
					input.pad[0] &= ~INPUT_DOWN;
				}
				input.pad[0] |= INPUT_UP;
				sdl_joystick.ystatus = 0;
				break;
			}

			/* axis is centered */
			if(sdl_joystick.ystatus == 2) {
				input.pad[0] &= ~INPUT_DOWN;
			}
			if(sdl_joystick.ystatus == 0) {
				input.pad[0] &= ~INPUT_UP;
			}
			sdl_joystick.ystatus = 1;
			break;
		}
		break;

	case SDL_JOYBUTTONDOWN:
		if(event.jbutton.button == sdl_joystick.map_b1) {
			input.pad[0] |= INPUT_BUTTON1;
		}
    else if(event.jbutton.button == sdl_joystick.map_b2) {
			input.pad[0] |= INPUT_BUTTON2;
		}
    else if(event.jbutton.button == sdl_joystick.map_start) {
			input.system |= (IS_GG) ? INPUT_START : INPUT_PAUSE;
		}
		break;

	case SDL_JOYBUTTONUP:
		if(event.jbutton.button == sdl_joystick.map_b1) {
			input.pad[0] &= ~INPUT_BUTTON1;
		}
    else if (event.jbutton.button == sdl_joystick.map_b2) {
			input.pad[0] &= ~INPUT_BUTTON2;
		}
    else if (event.jbutton.button == sdl_joystick.map_start) {
			input.system &= (IS_GG) ? ~INPUT_START : ~INPUT_PAUSE;
		}
		break;
	}
}


static int sdlsms_sync_init(int fullspeed)
{
  if(SDL_InitSubSystem(SDL_INIT_TIMER|SDL_INIT_EVENTTHREAD) < 0) {
    printf("ERROR: %s.\n", SDL_GetError());
    return 0;
  }
  sdl_sync.ticks_starting = SDL_GetTicks();
  if(fullspeed)
    sdl_sync.sem_sync = NULL;
  else
    sdl_sync.sem_sync = SDL_CreateSemaphore(0);
  return 1;
}


/* globals */

int sdlsms_init(const t_config* pcfg)
{
  rom_filename = pcfg->game_name;
  if(load_rom(rom_filename) == 0) {
    printf("ERROR: can't load `%s'.\n", pcfg->game_name);
    return 0;
  }
  else printf("Loaded `%s'.\n", pcfg->game_name);

  printf("Initializing SDL... ");
  if(SDL_Init(0) < 0) {
    printf("ERROR: %s.\n", SDL_GetError());
    return 0;
  }
  printf("Ok.\n");

  printf("Initializing SDL TIMER SUBSYSTEM... ");
  if(!sdlsms_sync_init(pcfg->fullspeed))
    return 0;
    
  printf("Initializing SDL CONTROLS SUBSYSTEM... ");
  sdlsms_controls_init();
  printf("Ok.\n");

  if(pcfg->joystick) {
    printf("Initializing SDL JOYSTICK SUBSYSTEM... ");
    if(smssdl_joystick_init())
      printf("Ok.\n");
  }

  printf("Initializing SDL VIDEO SUBSYSTEM... ");
  if(!sdlsms_video_init(pcfg->frameskip, pcfg->fullscreen, pcfg->filter))
    return 0;
  printf("Ok.\n");
  
  data_screen = SDL_CreateRGBSurface(SDL_HWSURFACE, 320, 240, 16, 0,0,0,0);
  
  font = gfx_tex_load_tga_from_array(fontarray);

  if(!pcfg->nosound) {
    printf("Initializing SDL SOUND SUBSYSTEM... ");
    if(!sdlsms_sound_init())
      return 0;
    printf("Ok.\n");
  }

  /* set up the virtual console emulation */
  SDL_LockSurface(sdl_video.surf_screen);
  printf("Initializing virtual console emulation... ");

  memset(&bitmap, 0, sizeof(t_bitmap));
  bitmap.width  = SMS_SCREEN_WIDTH;
  bitmap.height = SMS_SCREEN_HEIGHT;
  bitmap.depth  = 16;
  bitmap.pitch  = sdl_video.surf_screen->pitch;
  bitmap.data   = (unsigned char*)sdl_video.surf_screen->pixels;
  system_init(pcfg->nosound ? 0 : SOUND_FREQUENCY);
  load_sram(pcfg->game_name);
  SDL_UnlockSurface(sdl_video.surf_screen);
  printf("Ok.\n");

  return 1;
}


/* sync */

static Uint32 sdlsms_sync_timer_callback(Uint32 interval)
{
  SDL_SemPost(sdl_sync.sem_sync);
  return interval;
}


static void sdlsms_sync_close()
{
  float playtime = (float)(SDL_GetTicks() - sdl_sync.ticks_starting) / (float)1000;
  float avgfps = (float)sdl_video.frames_rendered / playtime;
  printf("[INFO] Average FPS = %.2f (%d%%).\n", avgfps, (int)(avgfps * 100 / MACHINE_FPS)); 
  printf("[INFO] Play time = %.2f sec.\n", playtime);
}


void sdlsms_emulate()
{
	Uint32 start;
	int quit = 0;
	printf("Starting emulation...\n");

	if(snd.enabled)
		SDL_PauseAudio(0);

	if(sdl_sync.sem_sync) 
	{
		SDL_SetTimer(50, sdlsms_sync_timer_callback);
	}

	while(!quit) 
	{
		start = SDL_GetTicks();
		/* pump SDL events */
		SDL_Event event;
		if(SDL_PollEvent(&event)) {
		  switch(event.type) {
		  case SDL_KEYUP:
		  case SDL_KEYDOWN:
			if(!sdlsms_controls_update(event.key.keysym.sym, event.type == SDL_KEYDOWN))
			  quit = 1;
			break;
		  case SDL_JOYAXISMOTION:
		  case SDL_JOYBUTTONDOWN:
		  case SDL_JOYBUTTONUP:
			smssdl_joystick_update(event);
			break;
		  case SDL_QUIT:
			quit = 1;
			break;
		  }
		}
		sdlsms_video_update();
		if(snd.enabled) sdlsms_sound_update(); 
		if(sdl_sync.sem_sync && sdl_video.frames_rendered % 3 == 0)
			SDL_SemWait(sdl_sync.sem_sync);
	}
}

void sdlsms_shutdown() 
{
  /* shutdown the virtual console emulation */
  printf("Shutting down virtual console emulation...\n");
  save_sram(rom_filename);
  system_reset();
  system_shutdown();

  printf("Shutting down SDL...\n");
  if(snd.enabled)
    sdlsms_sound_close();
  sdlsms_video_close();
  SDL_Quit();
}
