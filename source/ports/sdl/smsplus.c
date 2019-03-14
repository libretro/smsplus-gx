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

static gamedata_t gdata;

t_config option;

SDL_Surface* sdl_screen;
static SDL_Surface *sms_bitmap;

#ifdef SCALE2X_UPSCALER
static SDL_Surface* scale2x_buf;
static uint32_t dst_x, dst_w, dst_h;
#endif

extern SDL_Surface *font;
extern SDL_Surface *bigfontred;
extern SDL_Surface *bigfontwhite;

static uint8_t fullscreen = 1;
static uint8_t selectpressed = 0;
static uint8_t save_slot = 0;
static uint8_t quit = 0;
static const uint32_t upscalers_available = 1
#ifdef SCALE2X_UPSCALER
+1
#endif
;

static void video_update(void)
{
	SDL_LockSurface(sdl_screen);
	switch(fullscreen) 
	{
		// native res
        case 0:
		if(sms.console == CONSOLE_GG) 
			bitmap_scale(48,0,160,144,160,144,256,HOST_WIDTH_RESOLUTION-160,(uint16_t* restrict)sms_bitmap->pixels,(uint16_t* restrict)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-160)/2+(HOST_HEIGHT_RESOLUTION-144)/2*HOST_WIDTH_RESOLUTION);
		else 
			bitmap_scale(0,0,
			256,vdp.height,
			256,vdp.height,
			256, HOST_WIDTH_RESOLUTION-256,
			(uint16_t* restrict)sms_bitmap->pixels,(uint16_t* restrict)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-256)/2+(HOST_HEIGHT_RESOLUTION-vdp.height)/2*HOST_WIDTH_RESOLUTION);
		break;
		// Standard fullscreen
		case 1:
		if(sms.console == CONSOLE_GG) 
			upscale_160x144_to_320x240((uint32_t* restrict)sdl_screen->pixels, (uint32_t* restrict)sms_bitmap->pixels+24);
		else 
			upscale_SMS_to_320x240((uint32_t* restrict)sdl_screen->pixels, (uint32_t* restrict)sms_bitmap->pixels, vdp.height);
		break;
		case 2:
#ifdef SCALE2X_UPSCALER
		if(sms.console == CONSOLE_GG) 
		{
			dst_x = 96;
			dst_w = 320;
			dst_h = 144*2;
		}
		else
		{
			uint32_t hide_left = (vdp.reg[0] & 0x20) ? 1 : 0;
			dst_x = hide_left ? 16 : 0;
			dst_w = (hide_left ? 248 : 256)*2;
			dst_h = vdp.height*2;
		}
		scale2x(sms_bitmap->pixels, scale2x_buf->pixels, 512, 1024, 256, 240);
		bitmap_scale(dst_x,0,dst_w,dst_h,HOST_WIDTH_RESOLUTION,HOST_HEIGHT_RESOLUTION,512,0,(uint16_t* restrict)scale2x_buf->pixels,(uint16_t* restrict)sdl_screen->pixels);
#endif
		break;
	}
	SDL_UnlockSurface(sdl_screen);	
	
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
	if (sms.console == CONSOLE_COLECO)
    {
		coleco.keypad[0] = 0xff;
		coleco.keypad[1] = 0xff;
	}
	
	switch(k)
	{
		case SDLK_0:
		case SDLK_KP0:
			coleco.keypad[0] = 0;
		break;
		case SDLK_1:
		case SDLK_KP1:
			coleco.keypad[0] = 1;
		break;
		case SDLK_2:
		case SDLK_KP2:
			coleco.keypad[0] = 2;
		break;
		case SDLK_3:
		case SDLK_KP3:
			coleco.keypad[0] = 3;
		break;
		case SDLK_4:
		case SDLK_KP4:
			coleco.keypad[0] = 4;
		break;
		case SDLK_5:
		case SDLK_KP5:
			coleco.keypad[0] = 5;
		break;
		case SDLK_6:
		case SDLK_KP6:
			coleco.keypad[0] = 6;
		break;
		case SDLK_7:
		case SDLK_KP7:
			coleco.keypad[0] = 7;
		break;
		case SDLK_8:
		case SDLK_KP8:
			coleco.keypad[0] = 8;
		break;
		case SDLK_9:
		case SDLK_KP9:
			coleco.keypad[0] = 9;
		break;
		case SDLK_DOLLAR:
		case SDLK_KP_MULTIPLY:
			coleco.keypad[0] = 10;
		break;
		case SDLK_ASTERISK:
		case SDLK_KP_MINUS:
			coleco.keypad[0] = 11;
		break;
		case SDLK_ESCAPE:
			if(p)
				selectpressed = 1;
			else
				selectpressed = 0;
		break;
		case SDLK_RETURN:
			if(p)
				input.system |= (sms.console == CONSOLE_GG) ? INPUT_START : INPUT_PAUSE;
			else
				input.system &= (sms.console == CONSOLE_GG) ? ~INPUT_START : ~INPUT_PAUSE;
		break;
		case SDLK_UP:
			if(p)
				input.pad[0] |= INPUT_UP;
			else
				input.pad[0] &= ~INPUT_UP;
		break;	
		case SDLK_DOWN:
			if(p)
				input.pad[0] |= INPUT_DOWN;
			else
				input.pad[0] &= ~INPUT_DOWN;
		break;
		case SDLK_LEFT:
			if(p)
				input.pad[0] |= INPUT_LEFT;
			else
				input.pad[0] &= ~INPUT_LEFT;
		break;	
		case SDLK_RIGHT:
			if(p)
				input.pad[0] |= INPUT_RIGHT;
			else
				input.pad[0] &= ~INPUT_RIGHT;
		break;
		case SDLK_LCTRL:
			if(p)
				input.pad[0] |= INPUT_BUTTON1;
			else
				input.pad[0] &= ~INPUT_BUTTON1;
		break;	
		case SDLK_LALT:
			if(p)
				input.pad[0] |= INPUT_BUTTON2;
			else
				input.pad[0] &= ~INPUT_BUTTON2;
		break;
		default:
		break;
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
	char home_path[256];
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

static void Menu(void)
{
    int16_t pressed = 0;
    int16_t currentselection = 1;
    uint16_t miniscreenwidth = 140;
    uint16_t miniscreenheight = 135;
    SDL_Rect dstRect;
    SDL_Event Event;
    SDL_Surface* miniscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, miniscreenwidth, miniscreenheight, 16, sdl_screen->format->Rmask, sdl_screen->format->Gmask, sdl_screen->format->Bmask, sdl_screen->format->Amask);

    SDL_LockSurface(miniscreen);
    if(IS_GG)
        bitmap_scale(48,0,160,144,miniscreenwidth,miniscreenheight,256,0,(uint16_t* restrict)sms_bitmap->pixels,(uint16_t* restrict)miniscreen->pixels);
    else
        bitmap_scale(0,0,256,192,miniscreenwidth,miniscreenheight,256,0,(uint16_t* restrict)sms_bitmap->pixels,(uint16_t* restrict)miniscreen->pixels);
        
    SDL_UnlockSurface(miniscreen);
    char text[50];

    SDL_PollEvent(&Event);
    while (((currentselection != 1) && (currentselection != 5)) || (!pressed))
    {
        pressed = 0;
        SDL_FillRect( sdl_screen, NULL, 0 );

        dstRect.x = HOST_WIDTH_RESOLUTION-5-miniscreenwidth;
        dstRect.y = 30;
        SDL_BlitSurface(miniscreen,NULL,sdl_screen,&dstRect);

        gfx_font_print_center(sdl_screen,5,bigfontwhite,"SMSPlus-GX");

        if (currentselection == 1)
            gfx_font_print(sdl_screen,5,25,bigfontred,"Continue");
        else
            gfx_font_print(sdl_screen,5,25,bigfontwhite,"Continue");

        sprintf(text,"Save State %d",save_slot);

        if (currentselection == 2)
            gfx_font_print(sdl_screen,5,65,bigfontred,text);
        else
            gfx_font_print(sdl_screen,5,65,bigfontwhite,text);

        sprintf(text,"Load State %d",save_slot);

        if (currentselection == 3)
            gfx_font_print(sdl_screen,5,85,bigfontred,text);
        else
            gfx_font_print(sdl_screen,5,85,bigfontwhite,text);

        if (currentselection == 4)
        {
			switch(fullscreen)
			{
				case 0:
					gfx_font_print(sdl_screen,5,105,bigfontred,"Native");
				break;
				case 1:
					gfx_font_print(sdl_screen,5,105,bigfontred,"Stretched");
				break;
				case 2:
					gfx_font_print(sdl_screen,5,105,bigfontred,"EPX/Scale2X");
				break;
			}
        }
        else
        {
			switch(fullscreen)
			{
				case 0:
					gfx_font_print(sdl_screen,5,105,bigfontwhite,"Native");
				break;
				case 1:
					gfx_font_print(sdl_screen,5,105,bigfontwhite,"Stretched");
				break;
				case 2:
					gfx_font_print(sdl_screen,5,105,bigfontwhite,"EPX/Scale2X");
				break;
			}
        }

        if (currentselection == 5)
            gfx_font_print(sdl_screen,5,125,bigfontred,"Quit");
        else
            gfx_font_print(sdl_screen,5,125,bigfontwhite,"Quit");

        gfx_font_print_center(sdl_screen,sdl_screen->h-40-gfx_font_height(font),font,"SMS_SDL for the RS-97");
        gfx_font_print_center(sdl_screen,sdl_screen->h-30-gfx_font_height(font),font,"RS-97 port by gameblabla");
        gfx_font_print_center(sdl_screen,sdl_screen->h-20-gfx_font_height(font),font,"See full credits on github:");
        gfx_font_print_center(sdl_screen,sdl_screen->h-10-gfx_font_height(font),font,"https://github.com/gameblabla/sms_sdl");

        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
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
								if (fullscreen == 0)
									fullscreen = upscalers_available;
								else
									fullscreen--;
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
                                if (fullscreen > upscalers_available)
                                    fullscreen = 0;
                                break;
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
                case 4 :
					if (fullscreen == 0)
						fullscreen = upscalers_available;
					else
						fullscreen--;
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

		SDL_Flip(sdl_screen);
    }
    
    if (currentselection == 5)
    {
		quit = 1;
	}
        
	if (miniscreen) SDL_FreeSurface(miniscreen);
}

static void Cleanup(void)
{
#ifdef SCALE2X_UPSCALER
	if (scale2x_buf) SDL_FreeSurface(scale2x_buf);
#endif
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (sms_bitmap) SDL_FreeSurface(sms_bitmap);
	if (font) SDL_FreeSurface(font);
	if (bigfontwhite) SDL_FreeSurface(bigfontwhite);
	if (bigfontred) SDL_FreeSurface(bigfontred);
	
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
	
	memset(&option, 0, sizeof(option));
	
	option.fm = 1;
	option.spritelimit = 0;
	option.country = 0;
	option.tms_pal = 2;
	option.console = 0;
	option.nosound = 0;
	option.soundlevel = 2;
	
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
	
	SDL_Init(SDL_INIT_VIDEO);
	
	sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
	if (!sdl_screen)
	{
		fprintf(stdout, "Could not create display, exiting\n");	
		Cleanup();
		return 0;
	}
	
	sms_bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_WIDTH_SMS, 240, 16, 0, 0, 0, 0);
#ifdef SCALE2X_UPSCALER
	scale2x_buf = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_WIDTH_SMS*2, 480, 16, 0, 0, 0, 0);
#endif
	SDL_WM_SetCaption("SMS Plus GX", NULL);
	
	font = gfx_tex_load_tga_from_array(fontarray);
    bigfontwhite = gfx_tex_load_tga_from_array(bigfontwhitearray);
    bigfontred = gfx_tex_load_tga_from_array(bigfontredarray);
	
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
		// Refresh video data
		video_update();
		
		// Output audio
		Sound_Update();
		
		// Execute frame(s)
		system_frame(0);

		SDL_Flip(sdl_screen);
		
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
	
	Cleanup();
	
	return 0;
}
