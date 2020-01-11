#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <math.h>
#include <stdarg.h>

#include <libretro.h>

#ifdef _MSC_VER
#include "msvc_compat.h"
#endif

#include "shared.h"
#include "scaler.h"
#include "smsplus.h"

static gamedata_t gdata;

t_config option;

static uint16_t *sms_bitmap;

static char retro_save_directory[256];
static char retro_system_directory[256];

struct retro_perf_callback perf_cb;
static retro_get_cpu_features_t perf_get_cpu_features_cb = NULL;
static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
retro_audio_sample_batch_t audio_batch_cb;

static unsigned libretro_supports_bitmasks;
static unsigned width;
static unsigned height;

typedef struct
{
   unsigned retro;
   unsigned sms;
} sms_input_t;

static sms_input_t binds[6] =
{
   { RETRO_DEVICE_ID_JOYPAD_UP,    INPUT_UP },
   { RETRO_DEVICE_ID_JOYPAD_DOWN,  INPUT_DOWN },
   { RETRO_DEVICE_ID_JOYPAD_LEFT,  INPUT_LEFT },
   { RETRO_DEVICE_ID_JOYPAD_RIGHT, INPUT_RIGHT },
   { RETRO_DEVICE_ID_JOYPAD_B,     INPUT_BUTTON1 },
   { RETRO_DEVICE_ID_JOYPAD_A,     INPUT_BUTTON2 }
};

static void video_update(void)
{
   uint16_t *videobuf = NULL;
   uint32_t dst_x = 0;
   uint32_t dst_width = width;

   switch (sms.console)
   {
      case CONSOLE_GG:
         dst_x = 48;
         break;
      default:
         dst_x = (vdp.reg[0] & 0x20) ? 8 : 0;
         dst_width -= dst_x;
         break;
   }

   video_cb(sms_bitmap + dst_x, dst_width, height, 512);
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
   (void)sram;
   (void)slot_number;
   (void)mode;
}

static uint32_t sdl_controls_update_input(int k, int32_t p)
{
   /*if (sms.console == CONSOLE_COLECO)
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

   if (sms.console == CONSOLE_COLECO) input.system = 0;*/

   return 1;
}

static void bios_init()
{
   FILE *fd;
   char bios_path[1024];

   bios.rom = malloc(0x100000);
   bios.enabled = 0;

   snprintf(bios_path, sizeof(bios_path), "%s%s", gdata.biosdir, "bios.sms");

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

   snprintf(bios_path, sizeof(bios_path), "%s%s", gdata.biosdir, "BIOS_sordm5.bin");

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

   // Set the game name
   snprintf(gdata.gamename, sizeof(gdata.gamename), "%s", basename(filename));

   // Strip the file extension off
   for (i = strlen(gdata.gamename) - 1; i > 0; i--) {
      if (gdata.gamename[i] == '.') {
         gdata.gamename[i] = '\0';
         break;
      }
   }

   log_cb(RETRO_LOG_INFO, "gamename: %s\n", gdata.gamename);

   // Set up the sram directory
   snprintf(gdata.sramdir, sizeof(gdata.sramdir), "%s/", retro_save_directory);
   if (mkdir(gdata.sramdir, 0755) && errno != EEXIST) {
      fprintf(stderr, "Failed to create %s: %d\n", gdata.sramdir, errno);
   }

   log_cb(RETRO_LOG_INFO, "sram directory: %s\n", gdata.sramdir);

   // Set up the sram file
   snprintf(gdata.sramfile, sizeof(gdata.sramfile), "%s%s.sav", gdata.sramdir, gdata.gamename);

   // Set up the state directory
   snprintf(gdata.stdir, sizeof(gdata.stdir), "%s/", retro_save_directory);
   if (mkdir(gdata.stdir, 0755) && errno != EEXIST) {
      fprintf(stderr, "Failed to create %s: %d\n", gdata.stdir, errno);
   }

   log_cb(RETRO_LOG_INFO, "state directory: %s\n", gdata.stdir);

   // Set up the screenshot directory
   snprintf(gdata.scrdir, sizeof(gdata.scrdir), "%s/", retro_save_directory);
   if (mkdir(gdata.scrdir, 0755) && errno != EEXIST) {
      fprintf(stderr, "Failed to create %s: %d\n", gdata.scrdir, errno);
   }

   log_cb(RETRO_LOG_INFO, "screenshot directory: %s\n", gdata.scrdir);

   // Set up the sram directory
   snprintf(gdata.biosdir, sizeof(gdata.biosdir), "%s/", retro_system_directory);
   if (mkdir(gdata.biosdir, 0755) && errno != EEXIST) {
      fprintf(stderr, "Failed to create %s: %d\n", gdata.sramdir, errno);
   }

   log_cb(RETRO_LOG_INFO, "bios directory: %s\n", gdata.biosdir);
}

static void Cleanup(void)
{
   if (sms_bitmap) free(sms_bitmap);

   if (bios.rom) free(bios.rom);

   // Deinitialize audio and video output
   Sound_Close();

   // Shut down
   system_poweroff();
   system_shutdown();
}

// Libretro implementation

static void check_system_specs(void)
{
   unsigned level = 0;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

void retro_init(void)
{
   struct retro_log_callback log;
   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   bool achievements = true;
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);

   const char *dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
      sprintf(retro_system_directory, "%s", dir);

   log_cb(RETRO_LOG_INFO, "system directory: %s\n", retro_system_directory);

   dir = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
      sprintf(retro_save_directory, "%s", dir);

   log_cb(RETRO_LOG_INFO, "save directory: %s\n", retro_save_directory);

   enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_RGB565;
   if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565) && log_cb)
      log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");

   libretro_supports_bitmasks = 0;
   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = 1;

   check_system_specs();
}

void retro_reset(void)
{
   system_reset();
}

bool retro_load_game_special(unsigned id, const struct retro_game_info *info, size_t size)
{
   return false;
}

static void check_variables(void)
{
   struct retro_variable var = { 0 };
}

bool retro_load_game(const struct retro_game_info *info)
{
   if (!info)
      return false;

   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Button 1" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Button 2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },

      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   check_variables();

   smsp_gamedata_set((char*)info->path);

   memset(&option, 0, sizeof(option));

   option.fullscreen = 1;
   option.fm = 1;
   option.spritelimit = 1;
   option.tms_pal = 2;
   option.nosound = 0;
   option.soundlevel = 2;

   option.country = 0;
   option.console = 0;

   strcpy(option.game_name, info->path);

   // Force Colecovision mode if extension is .col
   if (strcmp(strrchr(info->path, '.'), ".col") == 0)
      option.console = 6;
   // Sometimes Game Gear games are not properly detected, force them accordingly
   else if (strcmp(strrchr(info->path, '.'), ".gg") == 0)
      option.console = 3;
   // Force M5
   else if (strcmp(strrchr(info->path, '.'), ".m5") == 0)
      option.console = 7;

   // Load ROM
   if (!load_rom(info->path))
   {
      fprintf(stderr, "Error: Failed to load %s.\n", info->path);
      Cleanup();
      return 0;
   }

   strcpy(option.game_name, info->path);

   width =  (sms.console == CONSOLE_GG) ? VIDEO_WIDTH_GG  : VIDEO_WIDTH_SMS;
   height = (sms.console == CONSOLE_GG) ? VIDEO_HEIGHT_GG : VIDEO_HEIGHT_SMS;

   sms_bitmap = (uint16_t*)malloc(VIDEO_WIDTH_SMS * VIDEO_HEIGHT_SMS * sizeof(uint16_t));

   fprintf(stdout, "CRC : %08X\n", cart.crc);

   // Set parameters for internal bitmap
   bitmap.width = VIDEO_WIDTH_SMS;
   bitmap.height = VIDEO_HEIGHT_SMS;
   bitmap.depth = 16;
   bitmap.granularity = 2;
   bitmap.data = (uint8_t *)sms_bitmap;
   bitmap.pitch = 512;
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

   return 1;
}

void retro_unload_game(void)
{
   Cleanup();
}

void retro_run(void)
{
   unsigned i = 0;
   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
   
   input_poll_cb();

   input.pad[0] = 0;
   input.system = 0;

   for (i = 0; i < 6; i++)
      input.pad[0] |= (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, binds[i].retro)) ? binds[i].sms : 0;

   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
      input.system |= (sms.console == CONSOLE_GG) ? INPUT_START : INPUT_PAUSE;

   /* Execute frame(s) */
   system_frame(0);

   /* Refresh video data */
   video_update();

   /* Output audio */
   Sound_Update();
}

void retro_get_system_info(struct retro_system_info *info)
{
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif

   memset(info, 0, sizeof(*info));
   info->library_name = APP_NAME;
   info->library_version = APP_VERSION GIT_VERSION;
   info->need_fullpath = true;
   info->valid_extensions = "sms|bin|rom|col|gg";
   info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));

   info->timing.fps = 60.0;
   info->timing.sample_rate = 44100.0;
   info->geometry.base_width = 256;
   info->geometry.base_height = 192;
   info->geometry.max_width = 256;
   info->geometry.max_height = 192;
   info->geometry.aspect_ratio = 4.0 / 3.0;
}

void retro_deinit()
{
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC; // FIXME: Regions for other cores.
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned in_port, unsigned device)
{
   if (in_port)
      return;
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data, size_t size)
{
   return 0;
}

bool retro_unserialize(const void *data, size_t size)
{
   return 0;
}

void *retro_get_memory_data(unsigned type)
{
   switch (type)
   {
      case RETRO_MEMORY_SYSTEM_RAM:
         return sms.wram;
      case RETRO_MEMORY_SAVE_RAM:
         return cart.sram;
   }

   return NULL;
}

size_t retro_get_memory_size(unsigned type)
{
   switch (type)
   {
      case RETRO_MEMORY_SYSTEM_RAM:
         return 0x2000;
      case RETRO_MEMORY_SAVE_RAM:
         return 0x8000;
   }

   return 0;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
}
