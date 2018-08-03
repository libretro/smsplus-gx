
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
#include <fcntl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "saves.h"
#include "scaler.h"
#include "shared.h"
#include "sdlsms.h"
#include "text_gui.h"
#include "bigfontwhite.h"
#include "bigfontred.h"
#include "font.h"

uint8_t selectpressed = 0,startpressed = 0;
static uint8_t quit = 0;
static int8_t showfps = -1;
static int8_t fullscreen = 1;
static SDL_mutex *sound_mutex;
static SDL_cond *sound_cv;
static const char *rom_filename;
static uint16_t fps = 0;
static uint32_t framecounter = 0;
static int32_t time1 = 0;
t_sdl_video sdl_video;
static t_sdl_sound sdl_sound;
static t_sdl_controls sdl_controls;
static t_sdl_sync sdl_sync;

static char buf[100];
#define SLASH '/'

char homedir[512];
char configname[512];

extern SDL_Surface *font;
extern SDL_Surface *bigfontred;
extern SDL_Surface *bigfontwhite;

/* video */

/* alekmaul's scaler taken from mame4all */
void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t *src, uint16_t *dst)
{
    uint32_t W,H,ix,iy,x,y;
    x=startx<<16;
    y=starty<<16;
    W=newwidth;
    H=newheight;
    ix=(viswidth<<16)/W;
    iy=(visheight<<16)/H;

    do 
    {
        uint16_t *buffer_mem=&src[(y>>16)*pitchsrc];
        W=newwidth; x=startx<<16;
        do 
        {
            *dst++=buffer_mem[x>>16];
            x+=ix;
        } while (--W);
        dst+=pitchdest;
        y+=iy;
    } while (--H);
}

/* end alekmaul's scaler taken from mame4all */

void Menu()
{
    int16_t pressed = 0;
    int16_t currentselection = 1;
    uint16_t miniscreenwidth = 140;
    uint16_t miniscreenheight = 135;
    SDL_Rect dstRect;
    char *cmd = NULL;
    SDL_Event Event;
    SDL_Surface* miniscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, miniscreenwidth, miniscreenheight, 16, sdl_video.surf_screen->format->Rmask, sdl_video.surf_screen->format->Gmask, sdl_video.surf_screen->format->Bmask, sdl_video.surf_screen->format->Amask);

    SDL_LockSurface(miniscreen);
    if(IS_GG)
        bitmap_scale(48,24,160,144,miniscreenwidth,miniscreenheight,256,0,(uint16_t*)sdl_video.surf_bitmap->pixels,(uint16_t*)miniscreen->pixels);
    else
        bitmap_scale(0,0,256,192,miniscreenwidth,miniscreenheight,256,0,(uint16_t*)sdl_video.surf_bitmap->pixels,(uint16_t*)miniscreen->pixels);
        
    SDL_UnlockSurface(miniscreen);
    char text[50];

    SDL_PollEvent(&Event);
    while (((currentselection != 1) && (currentselection != 6)) || (!pressed))
    {
        pressed = 0;
        SDL_FillRect( sdl_video.surf_screen, NULL, 0 );

        dstRect.x = sdl_video.surf_screen->w-5-miniscreenwidth;
        dstRect.y = 30;
        SDL_BlitSurface(miniscreen,NULL,sdl_video.surf_screen,&dstRect);

        gfx_font_print_center(sdl_video.surf_screen,5,bigfontwhite,"SMS_SDL 0.9.4aR7.1");

        if (currentselection == 1)
            gfx_font_print(sdl_video.surf_screen,5,25,bigfontred,"Continue");
        else
            gfx_font_print(sdl_video.surf_screen,5,25,bigfontwhite,"Continue");

        sprintf(text,"Save State %d",sdl_controls.state_slot);

        if (currentselection == 2)
            gfx_font_print(sdl_video.surf_screen,5,65,bigfontred,text);
        else
            gfx_font_print(sdl_video.surf_screen,5,65,bigfontwhite,text);

        sprintf(text,"Load State %d",sdl_controls.state_slot);

        if (currentselection == 3)
            gfx_font_print(sdl_video.surf_screen,5,85,bigfontred,text);
        else
            gfx_font_print(sdl_video.surf_screen,5,85,bigfontwhite,text);

        if (currentselection == 4)
        {
            if (fullscreen == 1)
                gfx_font_print(sdl_video.surf_screen,5,105,bigfontred,"Stretched");
            else
				gfx_font_print(sdl_video.surf_screen,5,105,bigfontred,"Native");
        }
        else
        {
            if (fullscreen == 1)
                gfx_font_print(sdl_video.surf_screen,5,105,bigfontwhite,"Stretched");
            else
				gfx_font_print(sdl_video.surf_screen,5,105,bigfontwhite,"Native");
        }

        sprintf(text,"%s",showfps ? "Show fps on" : "Show fps off");

        if (currentselection == 5)
            gfx_font_print(sdl_video.surf_screen,5,125,bigfontred,text);
        else
            gfx_font_print(sdl_video.surf_screen,5,125,bigfontwhite,text);


        if (currentselection == 6)
            gfx_font_print(sdl_video.surf_screen,5,145,bigfontred,"Quit");
        else
            gfx_font_print(sdl_video.surf_screen,5,145,bigfontwhite,"Quit");

        gfx_font_print_center(sdl_video.surf_screen,sdl_video.surf_screen->h-40-gfx_font_height(font),font,"SMS_SDL for the RS-97");
        gfx_font_print_center(sdl_video.surf_screen,sdl_video.surf_screen->h-30-gfx_font_height(font),font,"RS-97 port by gameblabla");
        gfx_font_print_center(sdl_video.surf_screen,sdl_video.surf_screen->h-20-gfx_font_height(font),font,"Initial port: exmortis@yandex.ru,joyrider");
        gfx_font_print_center(sdl_video.surf_screen,sdl_video.surf_screen->h-10-gfx_font_height(font),font,"Scaler code: Alekmaul, Harteex: TGA,bin2h");

        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection == 0)
                            currentselection = 6;
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == 7)
                            currentselection = 1;
                        break;
                    case SDLK_LCTRL:
                    case SDLK_LALT:
                    case SDLK_RETURN:
                        pressed = 1;
                        break;
                    case SDLK_LEFT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                sdl_controls.state_slot--;
								if (sdl_controls.state_slot == 0)
									sdl_controls.state_slot = 1;
                                break;
                            case 4:
                                fullscreen--;
                                    if (fullscreen < 0)
                                        fullscreen = 1;
                                break;
                            case 5:
                                showfps = !showfps;
                                break;

                        }
                        break;
                    case SDLK_RIGHT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                sdl_controls.state_slot++;
								if (sdl_controls.state_slot == 10)
									sdl_controls.state_slot = 9;
                                break;
                            case 4:
                                fullscreen++;
                                if (fullscreen > 1)
                                    fullscreen = 0;
                                break;
                            case 5:
                                showfps = !showfps;
                                break;
                        }
                        break;


                }
            }
        }

        if (pressed)
        {
            switch(currentselection)
            {
                case 5:
                    showfps = !showfps;
                    break;
                case 4 :
                    fullscreen++;
                    if (fullscreen > 1)
                        fullscreen = 0;
                    break;
                case 2 :
                    if(save_state(rom_filename, sdl_controls.state_slot))
                        currentselection = 1;
                    break;
                case 3 :
					if(load_state(rom_filename, sdl_controls.state_slot))
						currentselection = 1;
				break;
            }
        }

		SDL_Flip(sdl_video.surf_screen);
		//++sdl_video.frames_rendered;
    }
    
    if (currentselection == 6)
        quit = 1;

}

static void sdlsms_video_take_screenshot()
{
    uint8_t status;
    char ssname[0x200];
    char *rom_name;

    if((rom_name = strrchr(rom_filename, SLASH)) == NULL)
		rom_name = (char*)rom_filename;
    sprintf(ssname, "%s/%s", homedir, rom_name);
    sprintf(strrchr(ssname, '.'), "-%03d.bmp", sdl_video.current_screenshot);
    ++sdl_video.current_screenshot;
    SDL_LockSurface(sdl_video.surf_screen);
    status = SDL_SaveBMP(sdl_video.surf_screen, ssname);
    SDL_UnlockSurface(sdl_video.surf_screen);
    if(status == 0)
    printf("[INFO] Screenshot written to '%s'.\n", ssname);
}

static void homedir_init()
{
    char *home = getenv("HOME");
    if (home)
    {
		snprintf(homedir, sizeof(homedir), "%s/.sms_sdl", home);
		mkdir(homedir, 0777);
		memset(configname, 0, 512);
		snprintf(configname, sizeof(configname),"%s/smsgg.cfg", homedir);
	}
}

static int sdlsms_video_init(int frameskip, int afullscreen, int filter)
{
    uint16_t screen_width, screen_height;
    uint32_t vidflagsvidflags = SDL_SWSURFACE;
    FILE *f;
    Uint32 vidflags = SDL_HWSURFACE;
  
    screen_width  = (IS_GG) ? GG_SCREEN_WIDTH  : SMS_SCREEN_WIDTH;
    screen_height = (IS_GG) ? GG_SCREEN_HEIGHT : SMS_SCREEN_HEIGHT;

    sdl_video.surf_bitmap = NULL;

    if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        printf("ERROR: %s.\n", SDL_GetError());
        return 0;
    }

    /* Create the 16 bits bitmap surface (RGB 565) */
    /* Even if we are in GameGear mode, we need to allocate space for a buffer of 256x192 */
    sdl_video.surf_bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, SMS_SCREEN_WIDTH, SMS_SCREEN_HEIGHT + FILTER_MARGIN_HEIGHT * 2, 16, 0xF800, 0x07E0, 0x001F, 0);
    if(!sdl_video.surf_bitmap) 
    {
		printf("ERROR: can't create surface: %s.\n", SDL_GetError());
		return 0;
    }
    
    sdl_video.surf_screen = SDL_SetVideoMode(320, 240, 16, vidflags);

    if(!sdl_video.surf_screen) 
    {
        printf("ERROR: can't set video mode (%dx%d): %s.\n", 320, 240, SDL_GetError());
        return 0;
    }

    sdl_video.bitmap_offset = sdl_video.surf_bitmap->pitch * FILTER_MARGIN_HEIGHT;

	SDL_ShowCursor(SDL_DISABLE);
    homedir_init();

    f = fopen(configname,"rb");
    if(f)
    {
        fread(&fullscreen,sizeof(int),1,f);
        fread(&sdl_controls.state_slot,sizeof(int),1,f);
        fread(&showfps,sizeof(int),1,f);
        fclose(f);
    }
    else
    {
        fullscreen = 1;
        sdl_controls.state_slot = 1;
        showfps = 0;
    }
	return 1;
}

static void sdlsms_video_finish_update()
{
    SDL_Rect dest;
	SDL_LockSurface(sdl_video.surf_screen);
	
	switch(fullscreen) 
	{
		case 1:
        case 2: // aspect ratio
		if(IS_GG) 
			upscale_160x144_to_320x240((uint32_t*)sdl_video.surf_screen->pixels, (uint32_t*)sdl_video.surf_bitmap->pixels+((24*256)+48)/2);
		else 
			/* (256*2) removes the 2 black pixels on the top, so effectively it's 256x190, not 256x192 */
			upscale_256x192_to_320x240((uint32_t*)sdl_video.surf_screen->pixels, (uint32_t*)sdl_video.surf_bitmap->pixels+(256*2));
		break;
        default: // native res
		if(IS_GG) 
			bitmap_scale(48,24,160,144,160,144,256,sdl_video.surf_screen->w-160,(uint16_t*)sdl_video.surf_bitmap->pixels,(uint16_t*)sdl_video.surf_screen->pixels+(sdl_video.surf_screen->w-160)/2+(sdl_video.surf_screen->h-144)/2*sdl_video.surf_screen->w);
		else 
			bitmap_scale(0,0,256,192,256,192,256,sdl_video.surf_screen->w-256,(uint16_t*)sdl_video.surf_bitmap->pixels,(uint16_t*)sdl_video.surf_screen->pixels+(sdl_video.surf_screen->w-256)/2+(sdl_video.surf_screen->h-192)/2*sdl_video.surf_screen->w);
		break;
	}
        
	SDL_UnlockSurface(sdl_video.surf_screen);
	if(showfps) 
	{
		snprintf(buf, sizeof(buf), "%d fps",fps);
		dest.x = 8;
		dest.y = 8;
		dest.w = 36;
		dest.h = 8;
		SDL_FillRect(sdl_video.surf_screen, &dest, 0);

		gfx_font_print(sdl_video.surf_screen, 8, 8, font, buf);
		framecounter++;
		if (SDL_GetTicks() - time1 > 1000)
		{
			fps=framecounter;
			framecounter=0;
			time1 = SDL_GetTicks();
		}
	}
	SDL_Flip(sdl_video.surf_screen);
}

static void sdlsms_video_update()
{
    int16_t skip_current_frame = 0;

    if(sdl_video.frame_skip > 1)
		skip_current_frame = (sdl_video.frames_rendered % sdl_video.frame_skip == 0) ? 0 : 1;

    if(!skip_current_frame) 
    {
        sms_frame(0);
        sdlsms_video_finish_update();
    } 
    else 
		sms_frame(1);

    ++sdl_video.frames_rendered;
}

static void sdlsms_video_close()
{
	FILE *f;
    if (sdl_video.surf_screen) 
    {
        SDL_FreeSurface(sdl_video.surf_screen);
        f = fopen(configname,"wb");
        if (f) 
        {
            fwrite(&fullscreen,sizeof(int),1,f);
            fwrite(&sdl_controls.state_slot,sizeof(int),1,f);
            fwrite(&showfps,sizeof(int),1,f);
            fclose(f);
        }
    }
    if(sdl_video.surf_bitmap) SDL_FreeSurface(sdl_video.surf_bitmap);
    printf("[INFO] Frames rendered = %lu.\n", sdl_video.frames_rendered);
}


/* sound */

static void sdlsms_sound_callback(void *userdata, Uint8 *stream, int len)
{
	if(sdl_sound.current_emulated_samples < len) 
	{
		memset(stream, 0, len);
	}
	else 
	{
		memcpy(stream, sdl_sound.buffer, len);
		/* loop to compensate desync */
		do 
		{
			sdl_sound.current_emulated_samples -= len;
		} while(sdl_sound.current_emulated_samples > 2 * len);
		memcpy(sdl_sound.buffer, (sdl_sound.current_pos - sdl_sound.current_emulated_samples), sdl_sound.current_emulated_samples);
		sdl_sound.current_pos = sdl_sound.buffer + sdl_sound.current_emulated_samples;
	}
}

static int sdlsms_sound_init()
{
	uint32_t n;
	SDL_AudioSpec as_desired, as_obtained;
	
	if(SDL_Init(SDL_INIT_AUDIO) < 0) 
	{
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
	uint32_t i;
	int16_t* p;
	SDL_LockAudio();
	p = (int16_t*)sdl_sound.current_pos;
	for(i = 0; i < snd.bufsize; ++i) 
	{
		*p = snd.buffer[0][i];
		++p;
		*p = snd.buffer[1][i];
		++p;
	}
	sdl_sound.current_pos = (char*)p;
	sdl_sound.current_emulated_samples += snd.bufsize * 2 * sizeof(int16_t);
	SDL_UnlockAudio();
}

static void sdlsms_sound_close()
{
	int32_t audio_status = SDL_AUDIO_PLAYING;
	SDL_PauseAudio(1);
	/* Loops until audio device is paused then safely exit */
	while (audio_status != SDL_AUDIO_PAUSED)
	{
		SDL_Delay(2);
		audio_status = SDL_GetAudioStatus();
	}
	SDL_CloseAudio();
	if (sdl_sound.buffer) free(sdl_sound.buffer);
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

static int sdlsms_controls_update(SDLKey k, int32_t p)
{
    if(!p) 
    {
        switch(k) 
        {
            case SDLK_3:
                sdlsms_video_take_screenshot();
                break;
			case SDLK_TAB:
				if(save_state(rom_filename, sdl_controls.state_slot))
				printf("[INFO] Saved state to slot #%d.\n", sdl_controls.state_slot);
				snprintf(buf, sizeof(buf), "Saved state slot %d", sdl_controls.state_slot);
			break;
			case SDLK_BACKSPACE:
				if(load_state(rom_filename, sdl_controls.state_slot))
				printf("[INFO] Loaded state from slot #%d.\n", sdl_controls.state_slot);
				snprintf(buf, sizeof(buf), "Loaded state slot %d", sdl_controls.state_slot);
			break;
			case SDLK_SPACE:
				sdl_controls.state_slot++;
				if(sdl_controls.state_slot > 9)
					sdl_controls.state_slot = 0;
				snprintf(buf, sizeof(buf), "State slot : %d", sdl_controls.state_slot);
				printf("[INFO] Slot changed to #%d.\n", sdl_controls.state_slot);
			break;
            case SDLK_F4:
            case SDLK_F5:
                sdl_video.frame_skip += k == SDLK_F4 ? -1 : 1;
                if(sdl_video.frame_skip > 0) sdl_video.frame_skip = 1;
                if(sdl_video.frame_skip == 1)
                    printf("[INFO] Frame skip disabled.\n");
                else
                    printf("[INFO] Frame skip set to %d.\n", sdl_video.frame_skip);
                break;
        }
    }

    if (k == sdl_controls.pad[0].start) 
    {
		if (p) 
			input.system |= (IS_GG) ? INPUT_START : INPUT_PAUSE;
		else  
			input.system &= (IS_GG) ? ~INPUT_START : ~INPUT_PAUSE;
    } 
    else if (k == sdl_controls.pad[0].up) 
    {
        if(p)
			input.pad[0] |= INPUT_UP;
        else 
			input.pad[0] &= ~INPUT_UP;
    } else if (k == sdl_controls.pad[0].down) 
    {
        if (p)
			input.pad[0] |= INPUT_DOWN;
        else 
			input.pad[0] &= ~INPUT_DOWN;
    } 
    else if (k == sdl_controls.pad[0].left) 
    {
        if (p)
			input.pad[0] |= INPUT_LEFT;
        else 
			input.pad[0] &= ~INPUT_LEFT;
    }
	else if (k == sdl_controls.pad[0].right) 
	{
        if (p)
			input.pad[0] |= INPUT_RIGHT;
        else 
			input.pad[0] &= ~INPUT_RIGHT;
    } 
    else if (k == sdl_controls.pad[0].b1) 
    {
        if (p)
			input.pad[0] |= INPUT_BUTTON1;
        else  input.pad[0] &= ~INPUT_BUTTON1;
    } 
    else if (k == sdl_controls.pad[0].b2) 
    {
        if (p)
			input.pad[0] |= INPUT_BUTTON2;
        else 
			input.pad[0] &= ~INPUT_BUTTON2;
    }

    if (k == SDLK_RETURN) startpressed = p;
    if (k == SDLK_ESCAPE || k == SDLK_END) selectpressed = p;
    
    return 1;
}

/* sync */

static int sdlsms_sync_init(int32_t fullspeed)
{
	if(SDL_InitSubSystem(SDL_INIT_TIMER|SDL_INIT_EVENTTHREAD) < 0) 
	{
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

/* globals */

int sdlsms_init(const t_config* pcfg)
{
    rom_filename = pcfg->game_name;
    if(load_rom(rom_filename) == 0) 
    {
        printf("ERROR: can't load `%s'.\n", pcfg->game_name);
        return 0;
    } 
    else 
		printf("Loaded `%s'.\n", pcfg->game_name);

    printf("Initializing SDL... ");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
        printf("ERROR: %s.\n", SDL_GetError());
        return 0;
    }
    printf("Ok.\n");

    printf("Initializing SDL CONTROLS SUBSYSTEM... ");
    sdlsms_controls_init();
    printf("Ok.\n");
    
    printf("Initializing SDL VIDEO SUBSYSTEM... ");
    if(!sdlsms_video_init(pcfg->frameskip, pcfg->fullscreen, pcfg->filter))
        return 0;
    font = gfx_tex_load_tga_from_array(fontarray);
    bigfontwhite = gfx_tex_load_tga_from_array(bigfontwhitearray);
    bigfontred = gfx_tex_load_tga_from_array(bigfontredarray);
    printf("Ok.\n");
    
    if(!pcfg->nosound) 
    {
        printf("Initializing SDL SOUND SUBSYSTEM... ");
        if(!sdlsms_sound_init()) return 0;
        printf("Ok.\n");
    }
    
	printf("Initializing SDL TIMER SUBSYSTEM... ");
	if(!sdlsms_sync_init(pcfg->fullspeed))
		return 0;

    /* set up the virtual console emulation */
    SDL_LockSurface(sdl_video.surf_bitmap);
    printf("Initializing virtual console emulation... ");
    
    sms.use_fm = 1;
    sms.country = TYPE_DOMESTIC;
    sms.save = pcfg->usesram;
    
    memset(&bitmap, 0, sizeof(t_bitmap));
    bitmap.width  = SMS_SCREEN_WIDTH;
    bitmap.height = SMS_SCREEN_HEIGHT;
    bitmap.depth  = sdl_video.surf_bitmap->format->BitsPerPixel;
    bitmap.pitch  = sdl_video.surf_bitmap->pitch;
    bitmap.data   = (unsigned char*)sdl_video.surf_bitmap->pixels + sdl_video.bitmap_offset;
    
    system_init(SOUND_FREQUENCY);
    load_sram(pcfg->game_name);
    SDL_UnlockSurface(sdl_video.surf_bitmap);
    
    printf("Ok.\n");

    return 1;
}

void sdlsms_emulate()
{
	uint32_t start;
    printf("Starting emulation...\n");

    if(snd.enabled) SDL_PauseAudio(0);
	if(sdl_sync.sem_sync) SDL_SetTimer(50, sdlsms_sync_timer_callback);

    while(!quit) 
    {
		/* pump SDL events */
		SDL_Event event;
        if(SDL_PollEvent(&event)) 
        {
            switch(event.type) 
            {
				case SDL_KEYUP:
				case SDL_KEYDOWN:
					if(!sdlsms_controls_update(event.key.keysym.sym, event.type == SDL_KEYDOWN))
						quit = 1;
				break;
				case SDL_QUIT:
					quit = 1;
				break;
            }
        }

        if(selectpressed) 
        {
			sdlsms_sync_close();
            Menu();
            SDL_FillRect(sdl_video.surf_screen, NULL, 0);
            input.system &= (IS_GG) ? ~INPUT_START : ~INPUT_PAUSE;
            selectpressed = 0;
            startpressed = 0;
            sdlsms_sync_init(0);
			if(sdl_sync.sem_sync) SDL_SetTimer(50, sdlsms_sync_timer_callback);
        }
        sdlsms_video_update();
        if(snd.enabled) sdlsms_sound_update();
        if(sdl_sync.sem_sync && sdl_video.frames_rendered % 3 == 0) SDL_SemWait(sdl_sync.sem_sync);
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
    if(snd.enabled) sdlsms_sound_close();
    sdlsms_video_close();
    
    sdlsms_sync_close();
    
    if(font) SDL_FreeSurface(font);

    if(bigfontred) SDL_FreeSurface(bigfontred);

    if(bigfontwhite) SDL_FreeSurface(bigfontwhite);

    SDL_Quit();
}
