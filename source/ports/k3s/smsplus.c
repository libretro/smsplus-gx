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
#include "text_gui.h"
#include "bigfontwhite.h"
#include "bigfontred.h"
#include "font.h"
#include "sound_output.h"

uint32_t frames_rendered = 0;
gamedata_t gdata;

t_config option;

SDL_Surface* sdl_screen, *sms_bitmap;
static SDL_Surface* img_background;
uint32_t countedFrames = 0;
uint32_t start;

static SDL_Joystick* joystick[2];

extern SDL_Surface *font;
extern SDL_Surface *bigfontred;
extern SDL_Surface *bigfontwhite;

int8_t fullscreen = 1;
uint8_t selectpressed = 0;
uint8_t save_slot = 0;
uint8_t showfps = 0;
uint8_t quit = 0;

static void video_update()
{
	SDL_Rect dst, dstT;
	
	SDL_LockSurface(sdl_screen);
	switch(fullscreen) 
	{
		case 3: // FullScreen (No cropping)
			if (sms.console == CONSOLE_GG) 
			{
				dst.x = 0;
				dst.y = 0;
				dst.w = sdl_screen->w;
				dst.h = sdl_screen->h;
				dstT.x = 0;
				dstT.y = 0;
				dstT.w = 160;
				dstT.h = 144;
			}
			else
			{
				dst.x = 0;
				dst.y = 0;
				dst.w = sdl_screen->w;
				dst.h = sdl_screen->h;
				dstT.x = 0;
				dstT.y = 0;
				dstT.w = VIDEO_WIDTH_SMS-dstT.x;
				dstT.h = vdp.height;
			}
		break;
		case 2: // Fullscreen (cropped)
			if (sms.console == CONSOLE_GG) 
			{
				dst.x = 0;
				dst.y = 0;
				dst.w = sdl_screen->w;
				dst.h = sdl_screen->h;
				dstT.x = 0;
				dstT.y = 0;
				dstT.w = 160;
				dstT.h = 144;
			}
			else
			{
				dst.x = 0;
				dst.y = 0;
				dst.w = sdl_screen->w;
				dst.h = sdl_screen->h;
				if ((bitmap.data[0] == bitmap.data[4]))
				{
					dstT.x = 8;
				}
				else
				{
					dstT.x = 0;
				}
				dstT.y = 0;
				dstT.w = VIDEO_WIDTH_SMS-dstT.x;
				dstT.h = vdp.height;
			}
		break;
		case 1: // Stretch (keep aspect ratio)
			if (sms.console == CONSOLE_GG) 
			{
				dst.x = 80;
				dst.y = 0;
				dst.w = 320;
				dst.h = 144*2;
				dstT.x = 0;
				dstT.y = 0;
				dstT.w = 160;
				dstT.h = 144;
			}
			else
			{
				dst.x = 80;
				dst.y = 0;
				dst.w = 640;
				dst.h = sdl_screen->h;
				dstT.x = 0;
				dstT.y = 0;
				dstT.w = VIDEO_WIDTH_SMS;
				dstT.h = vdp.height;
			}
		break;
        case 0: // native res
			if (sms.console == CONSOLE_GG) 
			{
				dst.x = 0;
				dst.y = 0;
				dst.w = 160*3;
				dst.h = 144*3;
				dstT.x = 0;
				dstT.y = 0;
				dstT.w = 160;
				dstT.h = 144;
			}
			else
			{
				dst.x = 148;
				dst.y = (480-(vdp.height*2))/2;
				dst.w = 512;
				dst.h = vdp.height*2;
				dstT.x = 0;
				dstT.y = 0;
				dstT.w = VIDEO_WIDTH_SMS;
				dstT.h = vdp.height;
			}
		break;
	}
	SDL_SoftStretch(sms_bitmap, &dstT, sdl_screen, &dst);
	SDL_UnlockSurface(sdl_screen);
}

void smsp_state(uint8_t slot, uint8_t mode)
{
	// Save and Load States
	int8_t stpath[PATH_MAX];
	snprintf(stpath, sizeof(stpath), "%s%s.st%d", gdata.stdir, gdata.gamename, slot);
	printf("Path state %s\n", stpath);
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

void system_manage_sram(uint8_t *sram, uint8_t slot, uint8_t mode) 
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
	if (sms.console == CONSOLE_COLECO)
    {
		coleco.keypad[0] = 0xff;
		coleco.keypad[1] = 0xff;
	}
	
	switch(k)
	{
		/* At least allow them to play on the lowest difficulties,
		 * Maybe this needs a better implementation ? */
		case SDLK_TAB:
			coleco.keypad[0] = 1;
		break;
		case SDLK_BACKSPACE:
			coleco.keypad[0] = 2;
		break;
		case SDLK_ESCAPE:
			if(p)
				selectpressed = 1;
		break;
		case SDLK_RETURN:
			if(p)
				input.system |= (sms.console == CONSOLE_GG) ? INPUT_START : INPUT_PAUSE;
		break;
		case SDLK_UP:
			if(p)
				input.pad[0] |= INPUT_UP;
		break;	
		case SDLK_DOWN:
			if(p)
				input.pad[0] |= INPUT_DOWN;
		break;
		case SDLK_LEFT:
			if(p)
				input.pad[0] |= INPUT_LEFT;
		break;	
		case SDLK_RIGHT:
			if(p)
				input.pad[0] |= INPUT_RIGHT;
		break;
		case SDLK_LCTRL:
			if(p)
				input.pad[0] |= INPUT_BUTTON1;
		break;	
		case SDLK_LALT:
			if(p)
				input.pad[0] |= INPUT_BUTTON2;
		break;
	}
	
	if (sms.console == CONSOLE_COLECO) input.system = 0;
	
	return 1;
}


static void bios_init()
{
	FILE *fd;
	int8_t bios_path[256];
	
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


static void smsp_gamedata_set(int8_t *filename) 
{
	// Set paths, create directories
	int8_t home_path[256];
	snprintf(home_path, sizeof(home_path), "%s/.smsplus/", getenv("HOME"));
	
	if (mkdir(home_path, 0755) && errno != EEXIST) {
		fprintf(stderr, "Failed to create %s: %d\n", home_path, errno);
	}
	
	// Set the game name
	snprintf(gdata.gamename, sizeof(gdata.gamename), "%s", basename(filename));
	
	// Strip the file extension off
	for (uint32_t i = strlen(gdata.gamename) - 1; i > 0; i--) {
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
	snprintf(gdata.sramfile, sizeof(gdata.sramfile), "%s%s%s.sav", home_path, gdata.sramdir, gdata.gamename);
	
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

void Menu()
{
    int16_t pressed = 0;
    int16_t currentselection = 1;
    uint16_t miniscreenwidth = 140;
    uint16_t miniscreenheight = 135;
    SDL_Rect dstRect;
    int8_t *cmd = NULL;
    SDL_Event Event;
    /* Seriously, we need that mess due to crappy framebuffer drivers on the RS-07... */
    SDL_Surface* miniscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, miniscreenwidth, miniscreenheight, 16, sdl_screen->format->Rmask, sdl_screen->format->Gmask, sdl_screen->format->Bmask, sdl_screen->format->Amask);
    SDL_Surface* black_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, sdl_screen->w, sdl_screen->h, 16, sdl_screen->format->Rmask, sdl_screen->format->Gmask, sdl_screen->format->Bmask, sdl_screen->format->Amask);
    SDL_Surface* final_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, sdl_screen->w, sdl_screen->h, 16, sdl_screen->format->Rmask, sdl_screen->format->Gmask, sdl_screen->format->Bmask, sdl_screen->format->Amask);
	SDL_FillRect( black_screen, NULL, 0 );

    SDL_LockSurface(miniscreen);
    if(IS_GG)
        bitmap_scale(48,0,160,144,miniscreenwidth,miniscreenheight,256,0,(uint16_t*)sms_bitmap->pixels,(uint16_t*)miniscreen->pixels);
    else
        bitmap_scale(0,0,256,192,miniscreenwidth,miniscreenheight,256,0,(uint16_t*)sms_bitmap->pixels,(uint16_t*)miniscreen->pixels);
        
    SDL_UnlockSurface(miniscreen);
    char text[50];

    SDL_PollEvent(&Event);
    while (((currentselection != 1) && (currentselection != 5)) || (!pressed))
    {
        pressed = 0;
		
        dstRect.x = sdl_screen->w-5-miniscreenwidth;
        dstRect.y = 30;
		if (!img_background) SDL_BlitSurface(black_screen,NULL,final_screen,NULL);
		else SDL_BlitSurface(img_background,NULL,final_screen,NULL);
        SDL_BlitSurface(miniscreen,NULL,final_screen,&dstRect);

        gfx_font_print_center(final_screen,22,bigfontwhite,"SMSPlus-GX");

        if (currentselection == 1)
            gfx_font_print(final_screen,5,25,bigfontred,"Continue");
        else
            gfx_font_print(final_screen,5,25,bigfontwhite,"Continue");

        sprintf(text,"Save State %d",save_slot);

        if (currentselection == 2)
            gfx_font_print(final_screen,5,65,bigfontred,text);
        else
            gfx_font_print(final_screen,5,65,bigfontwhite,text);

        sprintf(text,"Load State %d",save_slot);

        if (currentselection == 3)
            gfx_font_print(final_screen,5,85,bigfontred,text);
        else
            gfx_font_print(final_screen,5,85,bigfontwhite,text);

        if (currentselection == 4)
        {
			switch (fullscreen)
			{
				case 0:
					gfx_font_print(final_screen,5,105,bigfontred,"Native resolution");
				break;
				case 1:
					gfx_font_print(final_screen,5,105,bigfontred,"Keep Aspect Ratio");
				break;
				case 2:
					gfx_font_print(final_screen,5,105,bigfontred,"Fullscreen (Auto-Crop)");
				break;
				case 3:
					gfx_font_print(final_screen,5,105,bigfontred,"Fullscreen (No Crop)");
				break;
			}
        }
        else
        {
			switch (fullscreen)
			{
				case 0:
					gfx_font_print(final_screen,5,105,bigfontwhite,"Native resolution");
				break;
				case 1:
					gfx_font_print(final_screen,5,105,bigfontwhite,"Keep Aspect Ratio");
				break;
				case 2:
					gfx_font_print(final_screen,5,105,bigfontwhite,"Fullscreen (Auto-Crop)");
				break;
				case 3:
					gfx_font_print(final_screen,5,105,bigfontwhite,"Fullscreen (No Crop)");
				break;
			}
        }

        if (currentselection == 5)
            gfx_font_print(final_screen,5,125,bigfontred,"Quit");
        else
            gfx_font_print(final_screen,5,125,bigfontwhite,"Quit");

        gfx_font_print_center(final_screen,sdl_screen->h-80-gfx_font_height(font),font,"SMS_SDL for the RS-97");
        gfx_font_print_center(final_screen,sdl_screen->h-70-gfx_font_height(font),font,"RS-97 port by gameblabla");
        gfx_font_print_center(final_screen,sdl_screen->h-60-gfx_font_height(font),font,"See full credits on github:");
        gfx_font_print_center(final_screen,sdl_screen->h-50-gfx_font_height(font),font,"https://github.com/gameblabla/sms_sdl");
		


		for(uint8_t i = 0; i < 2; i++)
		{
			if (joystick[i] == NULL) joystick[i] = SDL_JoystickOpen(i);
		}

        while (SDL_PollEvent(&Event))
        {
			switch(Event.type)
			{
				case SDL_KEYDOWN:
				{
					switch(Event.key.keysym.sym)
					{
						case SDLK_UP:
							currentselection--;
							if (currentselection == 0)
								currentselection = 5;
							break;
						case SDLK_DOWN:
							currentselection++;
							if (currentselection == 6)
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
									fullscreen--;
										if (fullscreen < 0)
											fullscreen = 3;
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
									fullscreen++;
									if (fullscreen > 3)
										fullscreen = 0;
									break;
							}
							break;
					}
					break;
				}
				case SDL_JOYAXISMOTION:
					if (Event.jaxis.axis == 0)
					{
						if (Event.jaxis.value < -500)
						{
							switch(currentselection)
							{
								case 2:
								case 3:
									if (save_slot > 0) save_slot--;
									break;
								case 4:
									fullscreen--;
										if (fullscreen < 0)
											fullscreen = 3;
									break;

							}
						}
						else if (Event.jaxis.value > 500)
						{
							switch(currentselection)
							{
								case 2:
								case 3:
									save_slot++;
									if (save_slot == 10)
										save_slot = 9;
									break;
								case 4:
									fullscreen++;
									if (fullscreen > 3)
										fullscreen = 0;
									break;
							}
						}
					}
					else if (Event.jaxis.axis == 1)
					{
						if (Event.jaxis.value < -500)
						{
							currentselection--;
							if (currentselection == 0)
								currentselection = 5;
						}
						else if (Event.jaxis.value > 500)
						{
							currentselection++;
							if (currentselection == 6)
								currentselection = 1;
						}
					}
				break;
				case SDL_JOYBUTTONDOWN:
				switch(Event.jbutton.button)
				{
					case 1:
					case 2:
						pressed = 1;
					break;
				}
				break;
			}
        }
        
		SDL_JoystickUpdate();
		
        if (pressed)
        {
            switch(currentselection)
            {
                case 4 :
                    fullscreen++;
                    if (fullscreen > 3)
                        fullscreen = 0;
                    break;
                case 2 :
                    smsp_state(save_slot, 0);
					currentselection = 1;
                    break;
                case 3 :
					smsp_state(save_slot, 1);
					currentselection = 1;
				break;
            }
        }

		SDL_BlitSurface(final_screen, NULL, sdl_screen, NULL);
		SDL_Flip(sdl_screen);
		//++sdl_video.frames_rendered;
    }
    
    if (currentselection == 5)
        quit = 1;
	if (quit)
	{
		SDL_FreeSurface(miniscreen);
		SDL_FreeSurface(black_screen);
		SDL_FreeSurface(final_screen);
	}
        
}

int main (int argc, char *argv[]) 
{
	int16_t x_move = 0, y_move = 0;
	Uint8 *keystate;
	SDL_Event event;
	// Print Header
	fprintf(stdout, "%s %s\n", APP_NAME, APP_VERSION);
	
	if(argc < 2) 
	{
		fprintf(stderr, "Usage: ./smsplus [FILE]\n");
		exit(1);
	}
	
	memset(&option, 0, sizeof(option));
	
	option.fm = 1;
	option.spritelimit = 0;
	option.filter = -1;
	option.country = 0;
	option.overscan = 0;
	option.tms_pal = 2;
	option.console = 0;
	option.nosound = 0;
	
	smsp_gamedata_set(argv[1]);
	
	// Force Colecovision mode
	if (strcmp(strrchr(argv[1], '.'), ".col") == 0)
	{
		option.console = 6;
	}

	// Load ROM
	if(!load_rom(argv[1])) {
		fprintf(stderr, "Error: Failed to load %s.\n", argv[1]);
		Cleanup();
		return 0;
	}
	
	strcpy(option.game_name, argv[1]);

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
	SDL_JoystickEventState(SDL_ENABLE);
	sdl_screen = SDL_SetVideoMode(800, 480, 16, SDL_HWSURFACE);
	SDL_SetCursor(0);
	
	// Needed in order to clear the cursor
	SDL_FillRect( sdl_screen, NULL, 0 );
	SDL_Flip(sdl_screen);
	
	sms_bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_WIDTH_SMS, 240, 16, 0, 0, 0, 0);
	
	img_background = SDL_LoadBMP("background.bmp");
	
    font = gfx_tex_load_tga_from_array(fontarray);
    bigfontwhite = gfx_tex_load_tga_from_array(bigfontwhitearray);
    bigfontred = gfx_tex_load_tga_from_array(bigfontredarray);
	
	// Load ROM
	if(!load_rom(argv[1])) {
		fprintf(stderr, "Error: Failed to load %s.\n", argv[1]);
		exit(1);
	}
	
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
	
	frames_rendered = 0;
	
	// Loop until the user closes the window
	while (!quit) 
	{
		// Refresh video data
		video_update();
		
		// Output audio
		Sound_Update();
		
		// Execute frame(s)
		system_frame(0);

		++frames_rendered;
		
		SDL_Flip(sdl_screen);
		
		if (sms.console == CONSOLE_COLECO)
		{
			input.system = 0;
			coleco.keypad[0] = 0xff;
			coleco.keypad[1] = 0xff;
		}

		if (selectpressed == 1)
		{
            Menu();
            SDL_FillRect(sdl_screen, NULL, 0);
            input.system &= (IS_GG) ? ~INPUT_START : ~INPUT_PAUSE;
            selectpressed = 0;
		}

		keystate = SDL_GetKeyState(NULL);
	
		for(uint8_t i=0;i<2;i++)
		{
				if (joystick[i] == NULL) joystick[i] = SDL_JoystickOpen(i);
				
				x_move = SDL_JoystickGetAxis(joystick[i], 0);
				y_move = SDL_JoystickGetAxis(joystick[i], 1);

				if (x_move > 500 || (i == 0 && keystate[SDLK_RIGHT]))
					input.pad[i] |= INPUT_RIGHT;
				else
					input.pad[i] &= ~INPUT_RIGHT;
					
				if (x_move < -500 || (i == 0 && keystate[SDLK_LEFT]))
					input.pad[i] |= INPUT_LEFT;
				else
					input.pad[i] &= ~INPUT_LEFT;

				if (y_move > 500 || (i == 0 && keystate[SDLK_DOWN]))
					input.pad[i] |= INPUT_DOWN;
				else
					input.pad[i] &= ~INPUT_DOWN;

				if(y_move < -500 || (i == 0 && keystate[SDLK_UP]))
					input.pad[i] |= INPUT_UP;
				else
					input.pad[i] &= ~INPUT_UP;

				if(SDL_JoystickGetButton(joystick[i], 2) == SDL_PRESSED || (i == 0 && keystate[SDLK_LCTRL]))
					input.pad[i] |= INPUT_BUTTON1;
				else
					input.pad[i] &= ~INPUT_BUTTON1;
				
				if(SDL_JoystickGetButton(joystick[i], 1) == SDL_PRESSED || (i == 0 && keystate[SDLK_LALT]))
					input.pad[i] |= INPUT_BUTTON2;
				else
					input.pad[i] &= ~INPUT_BUTTON2;

				if(i == 0 && SDL_JoystickGetButton(joystick[i], 9) == SDL_PRESSED || keystate[SDLK_RETURN])
					input.system |= (sms.console == CONSOLE_GG) ? INPUT_START : INPUT_PAUSE;
				else
					input.system &= ~(sms.console == CONSOLE_GG) ? INPUT_START : INPUT_PAUSE;
					
				if(SDL_JoystickGetButton(joystick[i], 8) == SDL_PRESSED || keystate[SDLK_ESCAPE])
					selectpressed = 1;
		}

		SDL_JoystickUpdate();

		if(SDL_PollEvent(&event)) 
		{
			switch(event.type) 
			{
				case SDL_QUIT:
					quit = 1;
				return;
			}
		}
	}
	
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (sms_bitmap) SDL_FreeSurface(sms_bitmap);
	if (font) SDL_FreeSurface(font);
	if (bigfontwhite) SDL_FreeSurface(bigfontwhite);
	if (bigfontred) SDL_FreeSurface(bigfontred);
	
	for(uint8_t i=0;i<2;i++)
	{
		if (joystick[i]) SDL_JoystickClose(joystick[i]);
	}
	
	// Deinitialize audio and video output
	Sound_Close();
	
	SDL_Quit();

	// Shut down
	system_poweroff();
	system_shutdown();
	return 0;
}
