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

static gamedata_t gdata;

t_config option;

static SDL_Surface* sdl_screen, *backbuffer;
static SDL_Surface *sms_bitmap;

static char home_path[256];

static uint8_t selectpressed = 0;
static uint8_t save_slot = 0;
static uint8_t quit = 0;
static const uint32_t upscalers_available = 1;

static void video_update(void)
{
	uint32_t dst_x, dst_w, dst_h;
	SDL_LockSurface(sdl_screen);
	switch(option.fullscreen)
	{
		case 0:
		if(sms.console == CONSOLE_GG) 
			bitmap_scale(48,0,160,144,160,144,256,HOST_WIDTH_RESOLUTION-160,(uint16_t* restrict)sms_bitmap->pixels,(uint16_t* restrict)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-160)/2+(HOST_HEIGHT_RESOLUTION-144)/2*HOST_WIDTH_RESOLUTION);
		else
		{
			uint32_t hide_left = (vdp.reg[0] & 0x20) ? 1 : 0;
			dst_x = hide_left ? 8 : 0;
			dst_w = (hide_left ? 248 : 256);
			dst_h = vdp.height;
			bitmap_scale(dst_x,0,dst_w,dst_h,sdl_screen->w,sdl_screen->h,256,0,(uint16_t* restrict)sms_bitmap->pixels,(uint16_t* restrict)sdl_screen->pixels);
		}
		break;
		case 1:
		if(sms.console == CONSOLE_GG) 
		{
			dst_x = 48;
			dst_w = 160;
			dst_h = 144;
		}
		else
		{
			uint32_t hide_left = (vdp.reg[0] & 0x20) ? 1 : 0;
			dst_x = hide_left ? 8 : 0;
			dst_w = (hide_left ? 248 : 256);
			dst_h = vdp.height;
		}
		bitmap_scale(dst_x,0,dst_w,dst_h,sdl_screen->w,sdl_screen->h,256,0,(uint16_t* restrict)sms_bitmap->pixels,(uint16_t* restrict)sdl_screen->pixels);
		break;
	}
	SDL_UnlockSurface(sdl_screen);	
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
			input.pad[0] |= INPUT_UP;
		else
			input.pad[0] &= ~INPUT_UP;
	}
	else if (k == option.config_buttons[CONFIG_BUTTON_LEFT])
	{
		if (p)
			input.pad[0] |= INPUT_LEFT;
		else
			input.pad[0] &= ~INPUT_LEFT;
	}
	else if (k == option.config_buttons[CONFIG_BUTTON_RIGHT])
	{
		if (p)
			input.pad[0] |= INPUT_RIGHT;
		else
			input.pad[0] &= ~INPUT_RIGHT;
	}
	else if (k == option.config_buttons[CONFIG_BUTTON_DOWN])
	{
		if (p)
			input.pad[0] |= INPUT_DOWN;
		else
			input.pad[0] &= ~INPUT_DOWN;
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
	else if (k == SDLK_RCTRL || k == SDLK_ESCAPE)
	{
		if (p)
			selectpressed = 1;
		else
			selectpressed = 0;
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
	unsigned long i;
	// Set paths, create directories
	snprintf(home_path, sizeof(home_path), "%s/.smsplus/", getenv("HOME"));
	
	if (mkdir(home_path, 0755) && errno != EEXIST) {
		fprintf(stderr, "Failed to create %s: %d\n", home_path, errno);
	}
	
	// Set the game name
	snprintf(gdata.gamename, sizeof(gdata.gamename), "%s", basename(filename));
	
	// Strip the file extension off
	for (i = strlen(gdata.gamename) - 1; i > 0; i--) {
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
		case 273:
			return "DPAD UP";
		break;
		/* DOWN button */
		case 274:
			return "DPAD DOWN";
		break;
		/* LEFT button */
		case 276:
			return "DPAD LEFT";
		break;
		/* RIGHT button */
		case 275:
			return "DPAD RIGHT";
		break;
		/* A button */
		case 306:
			return "A button";
		break;
		/* B button */
		case 308:
			return "B button";
		break;
		/* X button */
		case 304:
			return "X button";
		break;
		/* Y button */
		case 32:
			return "Y button";
		break;
		/* L button */
		case 9:
			return "L Shoulder";
		break;
		/* R button */
		case 8:
			return "R Shoulder";
		break;
		/* Start */
		case 13:
			return "Start button";
		break;
		case 27:
			return "Select button";
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
	uint32_t exit_map = 0;
	
	uint8_t menu_config_input = 0;
	
	while(!(currentselection == -2 && pressed == 1))
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
							currentselection = 1;
						}
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        switch(menu_config_input)
                        {
							case 0:
								if (currentselection > 7)
									currentselection = 7;
							break;
							case 1:
								if (currentselection > 11)
									currentselection = 11;
							break;
						}
                        break;
                    case SDLK_LEFT:
						currentselection -= 8;
						if (currentselection < 1) currentselection = 1;
                    break;
                    case SDLK_RIGHT:
						if (menu_config_input == 1)
						{
							currentselection += 8;
							if (currentselection > 11) currentselection = 10;
						}
                    break;
                    case SDLK_LCTRL:
                    case SDLK_RETURN:
                        pressed = 1;
					break;
                    case SDLK_TAB:
                        menu_config_input = 0;
                        currentselection = 1;
					break;
                    case SDLK_BACKSPACE:
                        if (sms.console == CONSOLE_COLECO) 
                        {
							menu_config_input = 1;
						}
						currentselection = 1;
					break;
                    case SDLK_LALT:
                        pressed = 1;
                        currentselection = -2;
					break;
                    case SDLK_ESCAPE:
						option.config_buttons[currentselection - 1] = 0;
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
				case -2:
					/* Exits */
				break;
                default:
					SDL_FillRect( backbuffer, NULL, 0 );
					print_string("Press button for mapping", TextWhite, TextBlue, 24, 64, backbuffer->pixels);
					bitmap_scale(0,0,240,160,sdl_screen->w,sdl_screen->h,240,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
					SDL_Flip(sdl_screen);
					exit_map = 0;
					while( !exit_map )
					{
						while (SDL_PollEvent(&Event))
						{
							if (Event.type == SDL_KEYDOWN)
							{
								option.config_buttons[(currentselection - 1) + (menu_config_input ? 7 : 0) ] = Event.key.keysym.sym;
								exit_map = 1;
							}
						}
					}
				break;
            }
        }

		print_string("[A] : Map, [B] : Exit", TextWhite, TextBlue, 20, 8, backbuffer->pixels);
		
		if (menu_config_input == 0)
		{
			snprintf(text, sizeof(text), "UP : %s\n", Return_Text_Button(option.config_buttons[0]));
			if (currentselection == 1) print_string(text, TextRed, 0, 5, 25, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 25, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "DOWN : %s\n", Return_Text_Button(option.config_buttons[1]));
			if (currentselection == 2) print_string(text, TextRed, 0, 5, 45, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 45, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "LEFT : %s\n", Return_Text_Button(option.config_buttons[2]));
			if (currentselection == 3) print_string(text, TextRed, 0, 5, 65, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 65, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "RIGHT : %s\n", Return_Text_Button(option.config_buttons[3]));
			if (currentselection == 4) print_string(text, TextRed, 0, 5, 85, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 85, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "A : %s\n", Return_Text_Button(option.config_buttons[4]));
			if (currentselection == 5) print_string(text, TextRed, 0, 5, 105, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 105, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "B : %s\n", Return_Text_Button(option.config_buttons[5]));
			if (currentselection == 6) print_string(text, TextRed, 0, 5, 125, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 125, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "START : %s\n", Return_Text_Button(option.config_buttons[6]));
			if (currentselection == 7) print_string(text, TextRed, 0, 5, 145, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 145, backbuffer->pixels);
		}
		else if (menu_config_input == 1)
		{
			snprintf(text, sizeof(text), "[*] : %s\n", Return_Text_Button(option.config_buttons[7]));
			if (currentselection == 1) print_string(text, TextRed, 0, 5, 25, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 25, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[#] : %s\n", Return_Text_Button(option.config_buttons[8]));
			if (currentselection == 2) print_string(text, TextRed, 0, 5, 45, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 45, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[1] : %s\n", Return_Text_Button(option.config_buttons[9]));
			if (currentselection == 3) print_string(text, TextRed, 0, 5, 65, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 65, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[2] : %s\n", Return_Text_Button(option.config_buttons[10]));
			if (currentselection == 4) print_string(text, TextRed, 0, 5, 85, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 85, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[3] : %s\n", Return_Text_Button(option.config_buttons[11]));
			if (currentselection == 5) print_string(text, TextRed, 0, 5, 105, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 105, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[4] : %s\n", Return_Text_Button(option.config_buttons[12]));
			if (currentselection == 6) print_string(text, TextRed, 0, 5, 125, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 125, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[5] : %s\n", Return_Text_Button(option.config_buttons[13]));
			if (currentselection == 7) print_string(text, TextRed, 0, 5, 145, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 145, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[6] : %s\n", Return_Text_Button(option.config_buttons[14]));
			if (currentselection == 8) print_string(text, TextRed, 0, 5, 145, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 5, 145, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[7] : %s\n", Return_Text_Button(option.config_buttons[15]));
			if (currentselection == 9) print_string(text, TextRed, 0, 145, 25, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 145, 25, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[8] : %s\n", Return_Text_Button(option.config_buttons[16]));
			if (currentselection == 10) print_string(text, TextRed, 0, 145, 45, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 145, 45, backbuffer->pixels);
			
			snprintf(text, sizeof(text), "[9] : %s\n", Return_Text_Button(option.config_buttons[17]));
			if (currentselection == 11) print_string(text, TextRed, 0, 145, 65, backbuffer->pixels);
			else print_string(text, TextWhite, 0, 145, 65, backbuffer->pixels);
		}
			
		bitmap_scale(0,0,240,160,sdl_screen->w,sdl_screen->h,240,0,(uint16_t* restrict)backbuffer->pixels,(uint16_t* restrict)sdl_screen->pixels);
		SDL_Flip(sdl_screen);
	}
	
}

void Menu()
{
	char text[50];
    int16_t pressed = 0;
    int16_t currentselection = 1;
    SDL_Rect dstRect;
    SDL_Event Event;
    
    while (((currentselection != 1) && (currentselection != 7)) || (!pressed))
    {
        pressed = 0;
 		SDL_FillRect( backbuffer, NULL, 0 );

		print_string("SMS Plus GX", TextWhite, 0, 72, 15, backbuffer->pixels);
		
		if (currentselection == 1) print_string("Continue", TextBlue, 0, 5, 30, backbuffer->pixels);
		else  print_string("Continue", TextWhite, 0, 5, 30, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Load State %d", save_slot);
		
		if (currentselection == 2) print_string(text, TextBlue, 0, 5, 45, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 45, backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Save State %d", save_slot);
		
		if (currentselection == 3) print_string(text, TextBlue, 0, 5, 60, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 60, backbuffer->pixels);
		

        if (currentselection == 4)
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextBlue, 0, 5, 75, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Stretched", TextBlue, 0, 5, 75, backbuffer->pixels);
				break;
			}
        }
        else
        {
			switch(option.fullscreen)
			{
				case 0:
					print_string("Scaling : Native", TextWhite, 0, 5, 75, backbuffer->pixels);
				break;
				case 1:
					print_string("Scaling : Stretched", TextWhite, 0, 5, 75, backbuffer->pixels);
				break;
			}
        }

		snprintf(text, sizeof(text), "Sound volume : %d", option.soundlevel);
		
		if (currentselection == 5) print_string(text, TextBlue, 0, 5, 90, backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 90, backbuffer->pixels);
		
		if (currentselection == 6) print_string("Input remapping", TextBlue, 0, 5, 105, backbuffer->pixels);
		else print_string("Input remapping", TextWhite, 0, 5, 105, backbuffer->pixels);
		
		if (currentselection == 7) print_string("Quit", TextBlue, 0, 5, 125, backbuffer->pixels);
		else print_string("Quit", TextWhite, 0, 5, 125, backbuffer->pixels);
		
		print_string("By gameblabla, ekeeke", TextWhite, 0, 5, 145, backbuffer->pixels);

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
                                if (save_slot > 0) save_slot--;
							break;
                            case 4:
							option.fullscreen--;
							if (option.fullscreen < 0)
								option.fullscreen = upscalers_available;
							break;
							case 5:
								option.soundlevel--;
								if (option.soundlevel < 1)
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
									option.soundlevel = 1;
							break;
                        }
                        break;
					default:
					break;
                }
            }
            else if (Event.type == SDL_QUIT)
            {
				currentselection = 6;
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
    
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #ifdef SDL_TRIPLEBUF
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #endif
    
    if (currentselection == 7)
        quit = 1;
}


static void config_load()
{
	uint_fast8_t i;
	char config_path[256];
	snprintf(config_path, sizeof(config_path), "%s/config.cfg", home_path);
	FILE* fp;
	
	fp = fopen(config_path, "rb");
	if (fp)
	{
		fread(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
	else
	{
		/* Default mapping for the Bittboy in case loading configuration file fails */
		option.config_buttons[CONFIG_BUTTON_UP] = 273;
		option.config_buttons[CONFIG_BUTTON_DOWN] = 274;
		option.config_buttons[CONFIG_BUTTON_LEFT] = 276;
		option.config_buttons[CONFIG_BUTTON_RIGHT] = 275;
		
		option.config_buttons[CONFIG_BUTTON_BUTTON1] = 306;
		option.config_buttons[CONFIG_BUTTON_BUTTON2] = 308;
		
		option.config_buttons[CONFIG_BUTTON_START] = 13;
		
		for (i = 7; i < 19; i++)
		{
			option.config_buttons[i] = 0;
		}
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
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (sms_bitmap) SDL_FreeSurface(sms_bitmap);

	if (bios.rom) free(bios.rom);
	
	// Deinitialize audio and video output
	Sound_Close();
	
	SDL_Quit();

	// Shut down
	system_poweroff();
	system_shutdown();	
}

int main (int argc, char *argv[]) 
{
	SDL_Event event;
	// Print Header
	fprintf(stdout, "%s %s\n", APP_NAME, APP_VERSION);
	
	if(argc < 2) 
	{
		fprintf(stderr, "Usage: ./smsplus [FILE]\n");
		exit(1);
	}
	
	smsp_gamedata_set(argv[1]);
	
	memset(&option, 0, sizeof(option));
	
	option.fullscreen = 1;
	option.fm = 1;
	option.spritelimit = 1;
	option.country = 0;
	option.tms_pal = 2;
	option.nosound = 0;
	option.soundlevel = 2;
	
	config_load();
	
	option.console = 0;
	
	snprintf(option.game_name, sizeof(option.game_name), "%s", basename(argv[1]));

	// Force Colecovision mode if extension is .col
	if (strcmp(strrchr(argv[1], '.'), ".col") == 0) option.console = 6;
	// Sometimes Game Gear games are not properly detected, force them accordingly
	else if (strcmp(strrchr(argv[1], '.'), ".gg") == 0) option.console = 3;

	// Load ROM
	if(!load_rom(argv[1])) {
		fprintf(stderr, "Error: Failed to load %s.\n", argv[1]);
		Cleanup();
		return 0;
	}

	SDL_Init(SDL_INIT_VIDEO);
	
	sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
	if (!sdl_screen)
	{
		fprintf(stdout, "Could not create display, exiting\n");	
		Cleanup();
		return 0;
	}
	SDL_FillRect(sdl_screen, NULL, 0);
	SDL_Flip(sdl_screen);
	
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0, 0, 0, 0);
	
	sms_bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_WIDTH_SMS, 240, 16, 0, 0, 0, 0);
	SDL_WM_SetCaption("SMS Plus GX", NULL);
	
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
	{ 
		sms.use_fm = 1; 
	}
	
	bios_init();

	Sound_Init();
	
	// Initialize all systems and power on
	system_poweron();
	
	// Loop until the user closes the window
	while (!quit) 
	{
		// Execute frame(s)
		system_frame(0);
		
		// Refresh video data
		video_update();
		
		// Output audio
		Sound_Update();

		if (selectpressed == 1)
		{
            Menu();
            SDL_FillRect(sdl_screen, NULL, 0);
            input.system &= (IS_GG) ? ~INPUT_START : ~INPUT_PAUSE;
            selectpressed = 0;
		}

		if(SDL_PollEvent(&event)) 
		{
			switch(event.type) 
			{
				case SDL_KEYUP:
					sdl_controls_update_input(event.key.keysym.sym, 0);
				break;
				case SDL_KEYDOWN:
					sdl_controls_update_input(event.key.keysym.sym, 1);
				break;
				case SDL_QUIT:
					quit = 1;
				break;
			}
		}
	}
	
	config_save();
	Cleanup();
	
	return 0;
}
