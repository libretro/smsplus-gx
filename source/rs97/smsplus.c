#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <SDL/SDL.h>
#include <portaudio.h>

#include "shared.h"
#include "scaler.h"
#include "smsplus.h"
#include "text_gui.h"
#include "bigfontwhite.h"
#include "bigfontred.h"
#include "font.h"

static t_sdl_sound sdl_sound;
gamedata_t gdata;

t_config option;

SDL_Surface* sdl_screen, *sms_bitmap;
static int32_t frames_rendered = 0;
uint32_t countedFrames = 0;
uint32_t start;

extern SDL_Surface *font;
extern SDL_Surface *bigfontred;
extern SDL_Surface *bigfontwhite;

uint8_t fullscreen = 1;
uint8_t selectpressed = 0;
uint8_t save_slot = 0;
uint8_t showfps = 0;
uint8_t quit = 0;

#define SOUND_FREQUENCY 48000
#define SOUND_SAMPLES_SIZE 2048
static int16_t buffer_snd[SOUND_FREQUENCY * 2];

/* Sound */

PaStream *apu_stream;

static uint32_t Sound_Init()
{
	int32_t err;
	err = Pa_Initialize();
	
	PaStreamParameters outputParameters;
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice) {
		return EXIT_FAILURE;
	}

	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paInt16;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	err = Pa_OpenStream( &apu_stream, NULL, &outputParameters, SOUND_FREQUENCY, SOUND_SAMPLES_SIZE, paNoFlag, NULL, NULL);
	err = Pa_StartStream( apu_stream );
	
	option.sndrate = SOUND_FREQUENCY;
	
	return 1;
}

static void Sound_Update()
{
	uint32_t i;
	for (i = 0; i < (4 * (SOUND_FREQUENCY / snd.fps)); i++) 
	{
		buffer_snd[i * 2] = snd.output[1][i];
		buffer_snd[i * 2 + 1] = snd.output[0][i];
	}
	Pa_WriteStream( apu_stream, buffer_snd, SOUND_FREQUENCY / snd.fps );
}

static void Sound_Close()
{
	int32_t err;
	err = Pa_CloseStream( apu_stream );
	err = Pa_Terminate();
}

static void video_update()
{
	SDL_LockSurface(sdl_screen);
	switch(fullscreen) 
	{
		case 1:
        case 2: // aspect ratio
		if(sms.console == CONSOLE_GG) 
			upscale_160x144_to_320x480((uint32_t*)sdl_screen->pixels, (uint32_t*)sms_bitmap->pixels+24);
		else 
			upscale_SMS_to_320x480((uint32_t*)sdl_screen->pixels, (uint32_t*)sms_bitmap->pixels, vdp.height);
		break;
        default: // native res
		if(sms.console == CONSOLE_GG) 
			bitmap_scale(48,0,160,144,160,288,256,sdl_screen->w-160,(uint16_t*)sms_bitmap->pixels,(uint16_t*)sdl_screen->pixels+(sdl_screen->w-160)/2+(sdl_screen->h-288)/2*sdl_screen->w);
		else 
			bitmap_scale(0,0,256,vdp.height,256,(vdp.height*2),256,sdl_screen->w-256,(uint16_t*)sms_bitmap->pixels,(uint16_t*)sdl_screen->pixels+(sdl_screen->w-256)/2+(sdl_screen->h-(vdp.height*2))/2*sdl_screen->w);
		break;;
	}
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

static int sdl_controls_update_input(SDLKey k, int32_t p)
{
	if (sms.console == CONSOLE_COLECO)
    {
		input.system = 0;
		coleco.keypad[0] = 0xff;
		coleco.keypad[1] = 0xff;
	}
	
	switch(k)
	{
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
		case SDLK_LALT:
			if(p)
				input.pad[0] |= INPUT_BUTTON1;
			else
				input.pad[0] &= ~INPUT_BUTTON1;
		break;
		case SDLK_LCTRL:
			if(p)
				input.pad[0] |= INPUT_BUTTON2;
			else
				input.pad[0] &= ~INPUT_BUTTON2;
		break;	
	}
	return 1;
}


static void bios_init()
{
	FILE *fd;
	int8_t bios_path[256];
	
	bios.rom = malloc(0x100000);
	bios.enabled = 0;
	
	snprintf(bios_path, sizeof(bios_path), "%s%s", gdata.biosdir, "BIOS.sms");

	fd = fopen("bios.sms", "rb");
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
	
	fd = fopen("bios.col", "rb");
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
	for (int i = strlen(gdata.gamename) - 1; i > 0; i--) {
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
    SDL_Surface* miniscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, miniscreenwidth, miniscreenheight, 16, sdl_screen->format->Rmask, sdl_screen->format->Gmask, sdl_screen->format->Bmask, sdl_screen->format->Amask);
    SDL_Surface* menu_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0, 0, 0, 0);

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
        SDL_FillRect( menu_surf, NULL, 0 );

        dstRect.x = menu_surf->w-5-miniscreenwidth;
        dstRect.y = 30;
        SDL_BlitSurface(miniscreen,NULL,menu_surf,&dstRect);

        gfx_font_print_center(menu_surf,5,bigfontwhite,"SMSPlusGX");

        if (currentselection == 1)
            gfx_font_print(menu_surf,5,25,bigfontred,"Continue");
        else
            gfx_font_print(menu_surf,5,25,bigfontwhite,"Continue");

        sprintf(text,"Save State %d",save_slot);

        if (currentselection == 2)
            gfx_font_print(menu_surf,5,65,bigfontred,text);
        else
            gfx_font_print(menu_surf,5,65,bigfontwhite,text);

        sprintf(text,"Load State %d",save_slot);

        if (currentselection == 3)
            gfx_font_print(menu_surf,5,85,bigfontred,text);
        else
            gfx_font_print(menu_surf,5,85,bigfontwhite,text);

        if (currentselection == 4)
        {
            if (fullscreen == 1)
                gfx_font_print(menu_surf,5,105,bigfontred,"Stretched");
            else
				gfx_font_print(menu_surf,5,105,bigfontred,"Native");
        }
        else
        {
            if (fullscreen == 1)
                gfx_font_print(menu_surf,5,105,bigfontwhite,"Stretched");
            else
				gfx_font_print(menu_surf,5,105,bigfontwhite,"Native");
        }

        sprintf(text,"%s",showfps ? "Show fps on" : "Show fps off");

        if (currentselection == 5)
            gfx_font_print(menu_surf,5,125,bigfontred,"Quit");
        else
            gfx_font_print(menu_surf,5,125,bigfontwhite,"Quit");

        gfx_font_print_center(menu_surf,menu_surf->h-40-gfx_font_height(font),font,"SMS_SDL for the RS-97");
        gfx_font_print_center(menu_surf,menu_surf->h-30-gfx_font_height(font),font,"RS-97 port by gameblabla");
        gfx_font_print_center(menu_surf,menu_surf->h-20-gfx_font_height(font),font,"Initial port: exmortis@yandex.ru,joyrider");
        gfx_font_print_center(menu_surf,menu_surf->h-10-gfx_font_height(font),font,"Scaler code: Alekmaul, Harteex: TGA,bin2h");

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
                                fullscreen--;
                                    if (fullscreen < 0)
                                        fullscreen = 1;
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
                                if (fullscreen > 1)
                                    fullscreen = 0;
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
                case 4 :
                    fullscreen++;
                    if (fullscreen > 1)
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

		SDL_SoftStretch(menu_surf, NULL, sdl_screen, NULL);
		SDL_Flip(sdl_screen);
		//++sdl_video.frames_rendered;
    }
    
    if (menu_surf) SDL_FreeSurface(menu_surf);
	if (miniscreen) SDL_FreeSurface(miniscreen);
    
    if (currentselection == 5)
        quit = 1;

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
	option.spritelimit = 1;
	option.filter = -1;
	option.country = 0;
	option.overscan = 0;
	option.tms_pal = 2;
	option.console = 0;
	option.nosound = 0;
	
	strcpy(option.game_name, argv[1]);
	
	smsp_gamedata_set(argv[1]);
	
	// Check the type of ROM
	sms.console = strcmp(strrchr(argv[1], '.'), ".gg") ? CONSOLE_SMS : CONSOLE_GG;
	
	SDL_Init(SDL_INIT_VIDEO);
	sdl_screen = SDL_SetVideoMode(320, 480, 16, SDL_HWSURFACE);
	sms_bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_WIDTH_SMS, 240, 16, 0, 0, 0, 0);
	SDL_ShowCursor(0);
	
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
		sms.use_fm = 1; 
	
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
				return;
			}
		}
		SDL_Flip(sdl_screen);
	}
	
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (sms_bitmap) SDL_FreeSurface(sms_bitmap);
	if (font) SDL_FreeSurface(font);
	if (bigfontwhite) SDL_FreeSurface(bigfontwhite);
	if (bigfontred) SDL_FreeSurface(bigfontred);
	
	// Deinitialize audio and video output
	Sound_Close();
	
	SDL_Quit();

	// Shut down
	system_poweroff();
	system_shutdown();
	
	return 0;
}
