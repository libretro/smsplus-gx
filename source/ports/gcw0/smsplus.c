#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <SDL/SDL.h>

#include "shared.h"
#include "scaler.h"
#include "smsplus.h"
#include "font_drawing.h"
#include "sound_output.h"

#if !SDL_TRIPLEBUF
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

static SDL_Joystick * sdl_joy[3];
#define joy_commit_range 8192

static gamedata_t gdata;

t_config option;

static char home_path[256];

static uint_fast8_t joy_numb = 1;

static SDL_Surface* sdl_screen, *scale2x_buf;
static SDL_Surface *backbuffer;
extern SDL_Surface *font;
extern SDL_Surface *bigfontred;
extern SDL_Surface *bigfontwhite;
static SDL_Surface* miniscreen;
SDL_Surface *sms_bitmap;

static uint8_t selectpressed = 0;
static uint8_t save_slot = 0;
static uint8_t quit = 0;

static const int8_t upscalers_available = 2
#ifdef SCALE2X_UPSCALER
+1
#endif
;

static uint32_t width_hold = 256;
static uint32_t width_remember = 256;
static uint32_t width_remove = 0;
static uint_fast8_t remember_res_height;

static uint_fast8_t scale2x_res = 1;
static uint_fast8_t forcerefresh = 0;

static uint_fast8_t dpad_input[4] = {0, 0, 0, 0};

uint32_t update_window_size(uint32_t w, uint32_t h);

#ifdef RS97
static const char *KEEP_ASPECT_FILENAME = "/proc/jz/ipu";
#else
static const char *KEEP_ASPECT_FILENAME = "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio";
#endif

static inline uint_fast8_t get_keep_aspect_ratio()
{
	FILE *f = fopen(KEEP_ASPECT_FILENAME, "rb");
	if (!f) return false;
	char c;
	fread(&c, 1, 1, f);
	fclose(f);
	return c == 'Y';
}

static inline void set_keep_aspect_ratio(uint_fast8_t n)
{
	FILE *f = fopen(KEEP_ASPECT_FILENAME, "wb");
	if (!f) return;
	char c = n ? 'Y' : 'N';
	fwrite(&c, 1, 1, f);
	fclose(f);
}

static void Clear_video()
{
	SDL_FillRect(sdl_screen, NULL, 0);
	SDL_Flip(sdl_screen);
	SDL_FillRect(sdl_screen, NULL, 0);
	SDL_Flip(sdl_screen);
	#ifdef SDL_TRIPLEBUF
	SDL_FillRect(sdl_screen, NULL, 0);
	SDL_Flip(sdl_screen);
	#endif
}

static void video_update()
{
	uint8_t hide_left;
	SDL_Rect dst, dst2;
	width_hold = (vdp.reg[0] & 0x20) ? 248 : 256;
	width_remove = (vdp.reg[0] & 0x20) ? 8 : 0;

	if (remember_res_height != vdp.height || width_remember != width_hold || forcerefresh == 1)
	{
		remember_res_height = vdp.height;
		
		if (option.fullscreen == 3) scale2x_res = 2;
		else scale2x_res = 1;
		
		if(sms.console == CONSOLE_GG) 
		{
			update_window_size(160*scale2x_res, 144*scale2x_res);
		}
		else
		{
			update_window_size(width_hold*scale2x_res, vdp.height*scale2x_res);
		}
		forcerefresh = 0;
		width_remember = width_hold;
	}

	switch(option.fullscreen) 
	{
		// Native
        case 0: 
		if(sms.console == CONSOLE_GG) 
		{
			dst.x = 48;
			dst.w = 160;
			dst.h = 144;
		}
		else
		{
			dst.x = width_remove;
			dst.w = width_hold;
			dst.h = vdp.height;
		}
		dst.y = 0;
		dst2.x = (sdl_screen->w-(dst.w))/2;
		dst2.y = (sdl_screen->h-(dst.h))/2;
		SDL_BlitSurface(sms_bitmap,&dst,sdl_screen,&dst2);
		break;
        case 1:
        case 2:
			if(sms.console == CONSOLE_GG) 
			{
				dst.x = 48;
				dst.w = 160;
				dst.h = 144;
			}
			else
			{
				dst.x = width_remove;
				dst.y = 0;
				dst.w = width_hold;
				dst.h = vdp.height;
			}
			dst.y = 0;
			SDL_BlitSurface(sms_bitmap,&dst,sdl_screen,NULL);
		break;
		// Hqx
		case 3:
#ifdef SCALE2X_UPSCALER
		if(sms.console == CONSOLE_GG) 
		{
			dst.x = 96;
			dst.y = 0;
			dst.w = 320;
			dst.h = 288;
		}
		else
		{
			dst.x = width_remove*2;
			dst.y = 0;
			dst.w = width_hold*2;
			dst.h = vdp.height*2;
		}
		scale2x(sms_bitmap->pixels, scale2x_buf->pixels, 512, 1024, 256, 240);
		SDL_BlitSurface(scale2x_buf,&dst,sdl_screen,NULL);
#endif
		break;
	}
	SDL_Flip(sdl_screen);
}

void smsp_state(uint8_t slot_number, uint8_t mode)
{
	// Save and Load States
	char stpath[PATH_MAX];
	snprintf(stpath, sizeof(stpath), "%s%s.st%d", gdata.stdir, gdata.gamename, slot_number);
	FILE *fd;
	
	switch(mode) {
		case 0:
			fd = fopen(stpath, "wb");
			if (fd) {
				system_save_state(fd);
				fclose(fd);
			}
			break;
		
		case 1:
			fd = fopen(stpath, "rb");
			if (fd) {
				system_load_state(fd);
				fclose(fd);
			}
			break;
	}
}

void system_manage_sram(uint8_t *sram, uint8_t slot_number, uint8_t mode) 
{
	// Set up save file name
	FILE *fd;
	switch(mode) 
	{
		case SRAM_SAVE:
			if(sms.save) 
			{
				fd = fopen(gdata.sramfile, "wb");
				if (fd) 
				{
					fwrite(sram, 0x8000, 1, fd);
					fclose(fd);
				}
			}
			break;
		
		case SRAM_LOAD:
			fd = fopen(gdata.sramfile, "rb");
			if (fd) 
			{
				sms.save = 1;
				fread(sram, 0x8000, 1, fd);
				fclose(fd);
			}
			else
				memset(sram, 0x00, 0x8000);
			break;
	}
}

static uint32_t sdl_controls_update_input(SDLKey k, int32_t p)
{
	/* We can't use switch... case because we are not using constants */
	if (sms.console == CONSOLE_COLECO)
    {
		coleco.keypad[0] = 0xff;
		coleco.keypad[1] = 0xff;
		
		if (k == option.config_buttons[CONFIG_BUTTON_ONE]) coleco.keypad[0] = 1;
		else if (k == option.config_buttons[CONFIG_BUTTON_TWO]) coleco.keypad[0] = 2;
		else if (k == option.config_buttons[CONFIG_BUTTON_THREE]) coleco.keypad[0] = 3;
		else if (k == option.config_buttons[CONFIG_BUTTON_FOUR]) coleco.keypad[0] = 4;
		else if (k == option.config_buttons[CONFIG_BUTTON_FIVE]) coleco.keypad[0] = 5;
		else if (k == option.config_buttons[CONFIG_BUTTON_SIX]) coleco.keypad[0] = 6;
		else if (k == option.config_buttons[CONFIG_BUTTON_SEVEN]) coleco.keypad[0] = 7;
		else if (k == option.config_buttons[CONFIG_BUTTON_EIGHT]) coleco.keypad[0] = 8;
		else if (k == option.config_buttons[CONFIG_BUTTON_NINE]) coleco.keypad[0] = 9;
		else if (k == option.config_buttons[CONFIG_BUTTON_DOLLARS]) coleco.keypad[0] = 10;
		else if (k == option.config_buttons[CONFIG_BUTTON_ASTERISK]) coleco.keypad[0] = 11;
	}
	
	if (k == option.config_buttons[CONFIG_BUTTON_UP])
	{
		if (p)
		{
			input.pad[0] |= INPUT_UP;
			dpad_input[0] = 1;
		}
		else
		{
			input.pad[0] &= ~INPUT_UP;
			dpad_input[0] = 0;
		}
	}
	else if (k == option.config_buttons[CONFIG_BUTTON_LEFT])
	{
		if (p)
		{
			input.pad[0] |= INPUT_LEFT;
			dpad_input[1] = 1;
		}
		else
		{
			input.pad[0] &= ~INPUT_LEFT;
			dpad_input[1] = 0;
		}
	}
	else if (k == option.config_buttons[CONFIG_BUTTON_RIGHT])
	{
		if (p)
		{
			input.pad[0] |= INPUT_RIGHT;
			dpad_input[2] = 1;
		}
		else
		{
			input.pad[0] &= ~INPUT_RIGHT;
			dpad_input[2] = 0;
		}
	}
	else if (k == option.config_buttons[CONFIG_BUTTON_DOWN])
	{
		if (p)
		{
			input.pad[0] |= INPUT_DOWN;
			dpad_input[3] = 1;
		}
		else
		{
			input.pad[0] &= ~INPUT_DOWN;
			dpad_input[3] = 0;
		}
	}
	else if (k == option.config_buttons[CONFIG_BUTTON_BUTTON1])
	{
		if (p)
			input.pad[0] |= INPUT_BUTTON1;
		else
			input.pad[0] &= ~INPUT_BUTTON1;
	}
	else if (k == option.config_buttons[CONFIG_BUTTON_BUTTON2])
	{
		if (p)
			input.pad[0] |= INPUT_BUTTON2;
		else
			input.pad[0] &= ~INPUT_BUTTON2;
	}
	else if (k == option.config_buttons[CONFIG_BUTTON_START])
	{
		if (p)
			input.system |= (sms.console == CONSOLE_GG) ? INPUT_START : INPUT_PAUSE;
		else
			input.system &= (sms.console == CONSOLE_GG) ? ~INPUT_START : ~INPUT_PAUSE;
	}
	
	if (sms.console == CONSOLE_COLECO) input.system = 0;
	
	return 1;
}


static void bios_init()
{
	FILE *fd;
	char bios_path[256];
	
	bios.rom = malloc(0x100000);
	bios.enabled = 0;
	
	snprintf(bios_path, sizeof(bios_path), "%s%s", gdata.biosdir, "BIOS.sms");

	fd = fopen(bios_path, "rb");
	if(fd)
	{
		/* Seek to end of file, and get size */
		fseek(fd, 0, SEEK_END);
		uint32_t size = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		if (size < 0x4000) size = 0x4000;
		fread(bios.rom, size, 1, fd);
		bios.enabled = 2;  
		bios.pages = size / 0x4000;
		fclose(fd);
	}

	snprintf(bios_path, sizeof(bios_path), "%s%s", gdata.biosdir, "BIOS.col");
	
	fd = fopen(bios_path, "rb");
	if(fd)
	{
		/* Seek to end of file, and get size */
		fread(coleco.rom, 0x2000, 1, fd);
		fclose(fd);
	}
}


static void smsp_gamedata_set(char *filename) 
{
	// Set paths, create directories
	snprintf(home_path, sizeof(home_path), "%s/.smsplus/", getenv("HOME"));
	
	if (mkdir(home_path, 0755) && errno != EEXIST) {
		fprintf(stderr, "Failed to create %s: %d\n", home_path, errno);
	}
	
	// Set the game name
	snprintf(gdata.gamename, sizeof(gdata.gamename), "%s", basename(filename));
	
	// Strip the file extension off
	for (unsigned long i = strlen(gdata.gamename) - 1; i > 0; i--) {
		if (gdata.gamename[i] == '.') {
			gdata.gamename[i] = '\0';
			break;
		}
	}
	
	// Set up the sram directory
	snprintf(gdata.sramdir, sizeof(gdata.sramdir), "%ssram/", home_path);
	if (mkdir(gdata.sramdir, 0755) && errno != EEXIST) {
		fprintf(stderr, "Failed to create %s: %d\n", gdata.sramdir, errno);
	}
	
	// Set up the sram file
	snprintf(gdata.sramfile, sizeof(gdata.sramfile), "%s%s.sav", gdata.sramdir, gdata.gamename);
	
	// Set up the state directory
	snprintf(gdata.stdir, sizeof(gdata.stdir), "%sstate/", home_path);
	if (mkdir(gdata.stdir, 0755) && errno != EEXIST) {
		fprintf(stderr, "Failed to create %s: %d\n", gdata.stdir, errno);
	}
	
	// Set up the screenshot directory
	snprintf(gdata.scrdir, sizeof(gdata.scrdir), "%sscreenshots/", home_path);
	if (mkdir(gdata.scrdir, 0755) && errno != EEXIST) {
		fprintf(stderr, "Failed to create %s: %d\n", gdata.scrdir, errno);
	}
	
	// Set up the sram directory
	snprintf(gdata.biosdir, sizeof(gdata.biosdir), "%sbios/", home_path);
	if (mkdir(gdata.biosdir, 0755) && errno != EEXIST) {
		fprintf(stderr, "Failed to create %s: %d\n", gdata.sramdir, errno);
	}
	
}

static const char* Return_Text_Button(uint32_t button)
{
	switch(button)
	{
		/* UP button */
		case SDLK_UP:
			return "DPAD UP";
		break;
		/* DOWN button */
		case SDLK_DOWN:
			return "DPAD DOWN";
		break;
		/* LEFT button */
		case SDLK_LEFT:
			return "DPAD LEFT";
		break;
		/* RIGHT button */
		case SDLK_RIGHT:
			return "DPAD RIGHT";
		break;
		/* A button */
		case SDLK_LCTRL:
			return "A button";
		break;
		/* B button */
		case SDLK_LALT:
			return "B button";
		break;
		/* Y button */
		case SDLK_LSHIFT:
			return "Y button";
		break;
		/* X button */
		case SDLK_SPACE:
			return "X button";
		break;
		/* L button */
		case SDLK_TAB:
			return "L1";
		break;
		/* R button */
		case SDLK_BACKSPACE:
			return "R1";
		break;
		case SDLK_PAGEUP:
			return "L2";
		break;
		case SDLK_PAGEDOWN:
			return "R2";
		break;
		/* Power button */
		case SDLK_HOME:
			return "POWER";
		break;
		/* Brightness */
		case 34:
			return "Brightness";
		break;
		/* Volume - */
		case 38:
			return "Volume -";
		break;
		/* Volume + */
		case 233:
			return "Volume +";
		break;
		/* Start */
		case SDLK_RETURN:
			return "Start button";
		break;
		/* Select */
		case SDLK_ESCAPE:
			return "Select button";
		break;
		case 0:
			return "...";
		break;
		default:
			return "Unknown";
		break;
	}	
}


static const char* Return_Volume(uint32_t vol)
{
	switch(vol)
	{
		case 0:
			return "Mute";
		break;
		case 1:
			return "25 %";
		break;
		case 2:
			return "50 %";
		break;
		case 3:
			return "75 %";
		break;
		case 4:
			return "100 %";
		break;
		default:
			return "...";
		break;
	}	
}

static void Input_Remapping()
{
	SDL_Event Event;
	char text[50];
	uint32_t pressed = 0;
	int32_t currentselection = 1;
	int32_t exit_input = 0;
	uint32_t exit_map = 0;
	
	while(!exit_input)
	{
		pressed = 0;
		SDL_FillRect( backbuffer, NULL, 0 );
		
        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection < 1)
                        {
							if (sms.console == CONSOLE_COLECO)
							{
								currentselection = 18;
							}
							else currentselection = 7;
						}
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == 19 && sms.console == CONSOLE_COLECO)
                        {
							currentselection = 1;
						}
                        else if (currentselection == 8 && sms.console != CONSOLE_COLECO)
                        {
							currentselection = 1;
						}
                        break;
                    case SDLK_LCTRL:
                    case SDLK_RETURN:
                        pressed = 1;
					break;
                    case SDLK_LALT:
                        exit_input = 1;
					break;
                    case SDLK_LEFT:
						if (sms.console == CONSOLE_COLECO)
						{
							if (currentselection > 8) currentselection -= 9;
						}
					break;
                    case SDLK_RIGHT:
						if (sms.console == CONSOLE_COLECO)
						{
							if (currentselection < 10) currentselection += 9;
						}
					break;
					default:
					break;
                }
            }
        }

        if (pressed)
        {
            switch(currentselection)
            {
                default:
					SDL_FillRect( backbuffer, NULL, 0 );
					print_string("Please press button for mapping", TextWhite, TextBlue, 37, 108, backbuffer->pixels);
					bitmap_scale(0,0,320,240,sdl_screen->w,sdl_screen->h,320,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
					SDL_Flip(sdl_screen);
					exit_map = 0;
					while( !exit_map )
					{
						while (SDL_PollEvent(&Event))
						{
							if (Event.type == SDL_KEYDOWN)
							{
								if (Event.key.keysym.sym != SDLK_END && Event.key.keysym.sym != SDLK_HOME && Event.key.keysym.sym != SDLK_RCTRL)
								{
									option.config_buttons[currentselection - 1] = Event.key.keysym.sym;
									exit_map = 1;
								}
							}
						}
					}
				break;
            }
        }
		
		print_string("Input remapping", TextWhite, 0, 100, 10, backbuffer->pixels);
		
		print_string("Press [A] to map to a button", TextWhite, TextBlue, 50, 210, backbuffer->pixels);
		print_string("Press [B] to Exit", TextWhite, TextBlue, 85, 225, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "  UP  : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_UP]));
		if (currentselection == 1) print_string(text, TextRed, 0, 5, 25+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 25+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), " DOWN : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_DOWN]));
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 45+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 45+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), " LEFT : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_LEFT]));
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 65+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 65+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "RIGHT : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_RIGHT]));
		if (currentselection == 4) print_string(text, TextRed, 0, 5, 85+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 85+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "BTN 1 : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_BUTTON1]));
		if (currentselection == 5) print_string(text, TextRed, 0, 5, 105+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 105+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "BTN 2 : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_BUTTON2]));
		if (currentselection == 6) print_string(text, TextRed, 0, 5, 125+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 125+2, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "START : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_START]));
		if (currentselection == 7) print_string(text, TextRed, 0, 5, 145+2, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 145+2, backbuffer->pixels);
		
		if (sms.console == CONSOLE_COLECO)
		{
			snprintf(text, sizeof(text), " [*]  : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_DOLLARS]));
			if (currentselection == 8) print_string(text, TextRed, 0, 5, 165+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 165+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), " [#]  : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_ASTERISK]));
			if (currentselection == 9) print_string(text, TextRed, 0, 5, 185+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 185+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "  [1] : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_ONE]));
			if (currentselection == 10) print_string(text, TextRed, 0, 165, 25+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 165, 25+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "  [2] : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_TWO]));
			if (currentselection == 11) print_string(text, TextRed, 0, 165, 45+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 165, 45+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "  [3] : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_THREE]));
			if (currentselection == 12) print_string(text, TextRed, 0, 165, 65+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 165, 65+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "  [4] : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_FOUR]));
			if (currentselection == 13) print_string(text, TextRed, 0, 165, 85+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 165, 85+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "  [5] : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_FIVE]));
			if (currentselection == 14) print_string(text, TextRed, 0, 165, 105+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 165, 105+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "  [6] : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_SIX]));
			if (currentselection == 15) print_string(text, TextRed, 0, 165, 125+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 165, 125+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "  [7] : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_SEVEN]));
			if (currentselection == 16) print_string(text, TextRed, 0, 165, 145+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 165, 145+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "  [8] : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_EIGHT]));
			if (currentselection == 17) print_string(text, TextRed, 0, 165, 165+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 165, 165+2, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "  [9] : %s\n", Return_Text_Button(option.config_buttons[CONFIG_BUTTON_NINE]));
			if (currentselection == 18) print_string(text, TextRed, 0, 165, 185+2, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 165, 185+2, backbuffer->pixels);
		}
		
		bitmap_scale(0,0,320,240,sdl_screen->w,sdl_screen->h,320,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
		SDL_Flip(sdl_screen);
	}
	
}

static void Menu()
{
	char text[50];
    int16_t pressed = 0;
    int16_t currentselection = 1;
    SDL_Event Event;
    
    Sound_Close();
    
	if (SDL_MUSTLOCK(miniscreen)) SDL_LockSurface(miniscreen);
    if(IS_GG)
        bitmap_scale(48,0,160,144,sdl_screen->w,sdl_screen->h,256,0,(uint16_t* restrict)sms_bitmap->pixels,(uint16_t* restrict)miniscreen->pixels);
    else
        bitmap_scale(width_remove,0,width_hold,vdp.height,sdl_screen->w,sdl_screen->h,256,0,(uint16_t* restrict)sms_bitmap->pixels,(uint16_t* restrict)miniscreen->pixels);  
    if (SDL_MUSTLOCK(miniscreen)) SDL_UnlockSurface(miniscreen);
    
    SDL_SetAlpha(miniscreen, SDL_SRCALPHA, 128);
    
    while (((currentselection != 1) && (currentselection != 7)) || (!pressed))
    {
        pressed = 0;
 		SDL_FillRect( backbuffer, NULL, 0 );
        SDL_BlitSurface(miniscreen,NULL,backbuffer,NULL);
        
		if (SDL_MUSTLOCK(backbuffer)) SDL_LockSurface(backbuffer);

		print_string("SMS PLUS GX", TextWhite, 0, 105, 15, backbuffer->pixels);
		
		if (currentselection == 1) print_string("Continue", TextRed, 0, 5, 45, backbuffer->pixels);
		else  print_string("Continue", TextWhite, 0, 5, 45, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Load State %d", save_slot);
		
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 65, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 65, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Save State %d", save_slot);
		
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 85, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 85, backbuffer->pixels);
		
        if (currentselection == 4)
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Fit", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : Fullscreen", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
				case 3:
					print_string("Scaling : EPX/Scale2x", TextRed, 0, 5, 105, backbuffer->pixels);
				break;
			}
        }
        else
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Fit", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 2:
					print_string("Scaling : Fullscreen", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
				case 3:
					print_string("Scaling : EPX/Scale2x", TextWhite, 0, 5, 105, backbuffer->pixels);
				break;
			}
        }

		snprintf(text, sizeof(text), "Sound volume : %s", Return_Volume(option.soundlevel));
		
		if (currentselection == 5) print_string(text, TextRed, 0, 5, 125, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 125, backbuffer->pixels);
		
		if (currentselection == 6) print_string("Input remapping", TextRed, 0, 5, 145, backbuffer->pixels);
		else print_string("Input remapping", TextWhite, 0, 5, 145, backbuffer->pixels);
		
		if (currentselection == 7) print_string("Quit", TextRed, 0, 5, 165, backbuffer->pixels);
		else print_string("Quit", TextWhite, 0, 5, 165, backbuffer->pixels);

		print_string("Build " __DATE__ ", " __TIME__, TextWhite, 0, 5, 195, backbuffer->pixels);
		print_string("Based on SMS Plus by Charles Mcdonald", TextWhite, 0, 5, 210, backbuffer->pixels);
		print_string("Fork of SMS Plus GX by gameblabla", TextWhite, 0, 5, 225, backbuffer->pixels);
		
		if (SDL_MUSTLOCK(backbuffer)) SDL_UnlockSurface(backbuffer);

        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection == 0)
                            currentselection = 7;
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == 8)
                            currentselection = 1;
                        break;
                    case SDLK_LALT:
                    case SDLK_END:
                    case SDLK_ESCAPE:
						pressed = 1;
						currentselection = 1;
						break;
                    case SDLK_LCTRL:
                    case SDLK_RETURN:
                        pressed = 1;
                        break;
                    case SDLK_LEFT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                if (save_slot > 0) save_slot--;
							break;
                            case 4:
							option.fullscreen--;
							if (option.fullscreen < 0)
								option.fullscreen = upscalers_available;
							break;
							case 5:
								option.soundlevel--;
								if (option.soundlevel > 4)
									option.soundlevel = 4;
							break;
                        }
                        break;
                    case SDLK_RIGHT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                save_slot++;
								if (save_slot == 10)
									save_slot = 9;
							break;
                            case 4:
                                option.fullscreen++;
                                if (option.fullscreen > upscalers_available)
                                    option.fullscreen = 0;
							break;
							case 5:
								option.soundlevel++;
								if (option.soundlevel > 4)
									option.soundlevel = 0;
							break;
                        }
                        break;
					default:
					break;
                }
            }
            else if (Event.type == SDL_QUIT)
            {
				currentselection = 7;
				pressed = 1;
			}
        }

        if (pressed)
        {
            switch(currentselection)
            {
				case 6:
					Input_Remapping();
				break;
				case 5:
					option.soundlevel++;
					if (option.soundlevel > 4)
						option.soundlevel = 1;
				break;
                case 4 :
                    option.fullscreen++;
                    if (option.fullscreen > upscalers_available)
                        option.fullscreen = 0;
                    break;
                case 2 :
                    smsp_state(save_slot, 1);
					currentselection = 1;
                    break;
                case 3 :
					smsp_state(save_slot, 0);
					currentselection = 1;
				break;
				default:
				break;
            }
        }

		SDL_BlitSurface(backbuffer, NULL, sdl_screen, NULL);
		SDL_Flip(sdl_screen);
    }
    
	Clear_video();
    
    if (currentselection == 7)
        quit = 1;
	else
		Sound_Init();
}

static void Reset_Mapping()
{
	uint_fast8_t i;
	option.config_buttons[CONFIG_BUTTON_UP] = SDLK_UP;
	option.config_buttons[CONFIG_BUTTON_DOWN] = SDLK_DOWN;
	option.config_buttons[CONFIG_BUTTON_LEFT] = SDLK_LEFT;
	option.config_buttons[CONFIG_BUTTON_RIGHT] = SDLK_RIGHT;
		
	option.config_buttons[CONFIG_BUTTON_BUTTON1] = SDLK_LCTRL;
	option.config_buttons[CONFIG_BUTTON_BUTTON2] = SDLK_LALT;
		
	option.config_buttons[CONFIG_BUTTON_START] = SDLK_RETURN;
	
	/* This is for the Colecovision buttons. Don't set those to anything */
	for (i = 7; i < 19; i++)
	{
		option.config_buttons[i] = 0;
	}	
}

static void config_load()
{
	FILE* fp;
	char config_path[256];
	
	snprintf(config_path, sizeof(config_path), "%s/config.cfg", home_path);
	
	fp = fopen(config_path, "rb");
	if (fp)
	{
		fread(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
		
		/* Earlier versions had the config settings set to 0. If so then do reset mapping. */
		if (option.config_buttons[CONFIG_BUTTON_UP] == 0)
		{
			Reset_Mapping();
		}
	}
	else
	{
		/* Default mapping for the Bittboy in case loading configuration file fails */
		Reset_Mapping();
	}
}

static void config_save()
{
	char config_path[256];
	snprintf(config_path, sizeof(config_path), "%s/config.cfg", home_path);
	FILE* fp;
	
	fp = fopen(config_path, "wb");
	if (fp)
	{
		fwrite(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
}

static void Cleanup(void)
{
	uint_fast8_t i;
#ifdef SCALE2X_UPSCALER
	if (scale2x_buf) SDL_FreeSurface(scale2x_buf);
	scale2x_buf = NULL;
#endif
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);
	if (sms_bitmap) SDL_FreeSurface(sms_bitmap);
	if (miniscreen) SDL_FreeSurface(miniscreen);
	
	backbuffer = NULL;
	sms_bitmap = NULL;
	miniscreen = NULL;
	
	if (bios.rom) free(bios.rom);
	bios.rom = NULL;
	
	for(i=0;i<joy_numb;i++)
	{
		SDL_JoystickClose(sdl_joy[i]);
	}

	SDL_Quit();

	// Shut down
	system_poweroff();
	system_shutdown();	
}

#define SDL_FLAGS SDL_HWSURFACE

uint32_t update_window_size(uint32_t w, uint32_t h)
{

	if (option.fullscreen == 0)
	{
		sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_FLAGS);
	}
	else
	{
		sdl_screen = SDL_SetVideoMode(w, h, 16, SDL_FLAGS);
	}
			
	if (!sdl_screen)
	{
		printf("SDL_SetVideoMode error\n");
		return 1;
	}

	if (vdp.height == 0) remember_res_height = 240;
	
	return 0;
}

static void Force_IPU_Mode()
{
	if (option.fullscreen == 0)
		set_keep_aspect_ratio(1);
	else if (option.fullscreen == 1)
		set_keep_aspect_ratio(0);
}


int main (int argc, char *argv[]) 
{
	int32_t joy_axis[2] = {0, 0};
	uint_fast8_t i;
	SDL_Event event;
	
	if(argc < 2) 
	{
		fprintf(stderr, "Usage: ./smsplus [FILE]\n");
		return 0;
	}
	
	smsp_gamedata_set(argv[1]);
	
	memset(&option, 0, sizeof(option));
	
	option.fullscreen = 1;
	option.fm = 1;
	option.spritelimit = 1;
	option.tms_pal = 2;
	option.console = 0;
	option.nosound = 0;
	option.soundlevel = 2;
	
	config_load();

	option.console = 0;
	
	strcpy(option.game_name, argv[1]);
	
	// Force Colecovision mode if extension is .col
	if (strcmp(strrchr(argv[1], '.'), ".col") == 0) option.console = 6;
	// Sometimes Game Gear games are not properly detected, force them accordingly
	else if (strcmp(strrchr(argv[1], '.'), ".gg") == 0) option.console = 3;
	
	if (option.fullscreen < 0 && option.fullscreen > upscalers_available) option.fullscreen = 1;
	if (option.console != 3 && option.fullscreen > 1) option.fullscreen = 1;
	
	// Load ROM
	if(!load_rom(argv[1])) 
	{
		fprintf(stderr, "Error: Failed to load %s.\n", argv[1]);
		Cleanup();
		return 0;
	}
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
	
	if (update_window_size(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION) == 1)
	{
		fprintf(stderr, "Error: Failed to init video window\n");
		Cleanup();
		return 0;
	}
	
	sms_bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_WIDTH_SMS, 240, 16, 0, 0, 0, 0);
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0, 0, 0, 0);
	miniscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0, 0, 0, 0);
	SDL_ShowCursor(0);
	
	SDL_JoystickEventState(SDL_ENABLE);
	
	joy_numb = SDL_NumJoysticks();
	if (joy_numb > 3) joy_numb = 3;
	
	for(i=0;i<joy_numb;i++)
	{
		sdl_joy[i] = SDL_JoystickOpen(i);
	}

	Sound_Init();

#ifdef SCALE2X_UPSCALER
	scale2x_buf = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_WIDTH_SMS*2, 480, 16, 0, 0, 0, 0);
#endif
	
	fprintf(stdout, "CRC : %08X\n", cart.crc);
	
	// Set parameters for internal bitmap
	bitmap.width = VIDEO_WIDTH_SMS;
	bitmap.height = VIDEO_HEIGHT_SMS;
	bitmap.depth = 16;
	bitmap.granularity = 2;
	bitmap.data = (uint8_t *)sms_bitmap->pixels;
	bitmap.pitch = sms_bitmap->pitch;
	bitmap.viewport.w = VIDEO_WIDTH_SMS;
	bitmap.viewport.h = VIDEO_HEIGHT_SMS;
	bitmap.viewport.x = 0x00;
	bitmap.viewport.y = 0x00;
	
	//sms.territory = settings.misc_region;
	if (sms.console == CONSOLE_SMS || sms.console == CONSOLE_SMS2)
		sms.use_fm = 1; 
	
	bios_init();

	// Initialize all systems and power on
	system_poweron();
	
	Force_IPU_Mode();
	
	// Loop until the user closes the window
	while (!quit) 
	{
		while (SDL_PollEvent(&event)) 
		{
			switch(event.type) 
			{
				case SDL_KEYUP:
					switch(event.key.keysym.sym) 
					{
						case SDLK_HOME:
						case SDLK_RCTRL:
						case SDLK_ESCAPE:
							selectpressed = 1;
						break;
						default:
						break;
					}
					sdl_controls_update_input(event.key.keysym.sym, 0);
				break;
				case SDL_KEYDOWN:
					sdl_controls_update_input(event.key.keysym.sym, 1);
				break;
				case SDL_JOYAXISMOTION:
					switch (event.jaxis.axis)
					{
						case 0: /* X axis */
							joy_axis[0] = event.jaxis.value;
						break;
						case 1: /* Y axis */
							joy_axis[1] = event.jaxis.value;
						break;
					}
				break;
				case SDL_QUIT:
					quit = 1;
				break;
			}
		}
		
		if (joy_axis[0] > joy_commit_range) input.pad[0] |= INPUT_RIGHT;
		else if (joy_axis[0] < -joy_commit_range) input.pad[0] |= INPUT_LEFT;
		else if (dpad_input[1] == 0 && dpad_input[2] == 0)
		{
			input.pad[0] &= ~INPUT_LEFT;
			input.pad[0] &= ~INPUT_RIGHT;
		}
		
		if (joy_axis[1] > joy_commit_range) input.pad[0] |= INPUT_DOWN;
		else if (joy_axis[1] < -joy_commit_range) input.pad[0] |= INPUT_UP;
		else if (dpad_input[0] == 0 && dpad_input[3] == 0)
		{
			input.pad[0] &= ~INPUT_UP;
			input.pad[0] &= ~INPUT_DOWN;
		}

		// Execute frame(s)
		system_frame(0);
		
		// Refresh video data
		video_update();
		
		// Output audio
		Sound_Update();
		
		if (selectpressed == 1)
		{
			update_window_size(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION);
            Menu();
			Clear_video();
            input.system &= (IS_GG) ? ~INPUT_START : ~INPUT_PAUSE;
            selectpressed = 0;
            forcerefresh = 1;
			Force_IPU_Mode();
		}
	}
	
	/* Change it back to Normal */
	set_keep_aspect_ratio(1);
	
	config_save();
	Cleanup();
	
	return 0;
}
