#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <math.h>
#include <stdarg.h>

#include <libretro.h>
#include <streams/memory_stream.h>
#include <libretro_state.h>

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
static unsigned libretro_serialize_size;
static unsigned width;
static unsigned height;

#ifdef _WIN32
#define path_default_slash() "\\"
#define path_default_slash_c() '\\'
#else
#define path_default_slash() "/"
#define path_default_slash_c() '/'
#endif

#define MAX_BUTTONS 6

typedef struct
{
   unsigned retro;
   unsigned sms;
} sms_input_t;

static sms_input_t binds[MAX_BUTTONS] =
{
   { RETRO_DEVICE_ID_JOYPAD_UP,    INPUT_UP },
   { RETRO_DEVICE_ID_JOYPAD_DOWN,  INPUT_DOWN },
   { RETRO_DEVICE_ID_JOYPAD_LEFT,  INPUT_LEFT },
   { RETRO_DEVICE_ID_JOYPAD_RIGHT, INPUT_RIGHT },
   { RETRO_DEVICE_ID_JOYPAD_B,     INPUT_BUTTON1 },
   { RETRO_DEVICE_ID_JOYPAD_A,     INPUT_BUTTON2 }
};


static void get_basename(char *buf, const char *path, size_t size)
{
   char *base = strrchr(path, '/');
   if (!base)
      base = strrchr(path, '\\');

   if (base)
   {
      snprintf(buf, size, "%s", base);
      base = strrchr(buf, '.');
      if (base)
         *base = '\0';
   }
   else
      buf[0] = '\0';
}

static void get_basedir(char *buf, const char *path, size_t size)
{
   char *base;
   strncpy(buf, path, size - 1);
   buf[size - 1] = '\0';

   base = strrchr(buf, '/');
   if (!base)
      base = strrchr(buf, '\\');

   if (base)
      *base = '\0';
   else
      buf[0] = '\0';
}

static void video_update(void)
{
   uint16_t *videobuf = NULL;
   uint32_t dst_x = 0;
   unsigned dst_width = width;
   const size_t pitch = 512;

   switch (sms.console)
   {
      case CONSOLE_GG:
         dst_x = 48;
         break;
      default:
         dst_x = (vdp.reg[0] & 0x20) ? 8 : 0;
         dst_width = width - dst_x;
         break;
   }

   videobuf = sms_bitmap + dst_x;
   video_cb(videobuf, dst_width, height, pitch);
}

void smsp_state(uint8_t slot_number, uint8_t mode)
{
   (void)slot_number;
   (void)mode;
}

void system_manage_sram(uint8_t *sram, uint8_t slot_number, uint8_t mode)
{
   (void)sram;
   (void)slot_number;
   (void)mode;
}

static uint32_t sdl_controls_update_input(int k, int32_t p)
{
   (void)k;
   (void)p;
   return 0;
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
      log_cb(RETRO_LOG_INFO, "bios loaded:      %s\n", bios_path);
   }

   snprintf(bios_path, sizeof(bios_path), "%s%s", gdata.biosdir, "BIOS.col");

   fd = fopen(bios_path, "rb");
   if(fd)
   {
      /* Seek to end of file, and get size */
      fread(coleco.rom, 0x2000, 1, fd);
      fclose(fd);
      log_cb(RETRO_LOG_INFO, "bios loaded:      %s\n", bios_path);
   }
}

static void smsp_gamedata_set(char *filename)
{
   unsigned long i;

   /* Set the game name */
   get_basename(gdata.gamename, filename, sizeof(gdata.gamename));

   /* Check and remove default slash from the beginning of the base name */
   if (gdata.gamename[0] == path_default_slash_c())
      snprintf(gdata.gamename, sizeof(gdata.gamename), "%s", gdata.gamename + 1);

   /* Set up the bios directory */
   snprintf(gdata.biosdir, sizeof(gdata.biosdir), "%s%c", retro_system_directory, path_default_slash_c());
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

static void update_input(void)
{
   unsigned i;
   bool startpressed = false;

   input_poll_cb();

   input.pad[0] = 0;
   input.system &= (sms.console == CONSOLE_GG) ? ~INPUT_START : ~INPUT_PAUSE;

   if (libretro_supports_bitmasks)
   {
      uint16_t ret = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);

      for (i = 0; i < MAX_BUTTONS; i++)
         input.pad[0] |= (ret & (1 << binds[i].retro)) ? binds[i].sms : 0;

      if (ret & (1 << RETRO_DEVICE_ID_JOYPAD_START))
         startpressed = true;
   }
   else
   {
      for (i = 0; i < MAX_BUTTONS; i++)
         input.pad[0] |= (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, binds[i].retro)) ? binds[i].sms : 0;

      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
         startpressed = true;
   }

   if (startpressed)
      input.system |= (sms.console == CONSOLE_GG) ? INPUT_START : INPUT_PAUSE;
}

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

   dir = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
      sprintf(retro_save_directory, "%s", dir);

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

   // Load ROM
   if (!load_rom_mem((const char*)info->data, info->size))
   {
      fprintf(stderr, "Error: Failed to load %s.\n", info->path);
      Cleanup();
      return 0;
   }

   width =  (sms.console == CONSOLE_GG) ? VIDEO_WIDTH_GG  : VIDEO_WIDTH_SMS;
   height = (sms.console == CONSOLE_GG) ? VIDEO_HEIGHT_GG : VIDEO_HEIGHT_SMS;

   sms_bitmap = (uint16_t*)malloc(VIDEO_WIDTH_SMS * VIDEO_HEIGHT_SMS * sizeof(uint16_t));

   log_cb(RETRO_LOG_INFO, "CRC :             0x%08X\n", cart.crc);
   log_cb(RETRO_LOG_INFO, "gamename:         %s\n", gdata.gamename);
   log_cb(RETRO_LOG_INFO, "system directory: %s\n", retro_system_directory);
   log_cb(RETRO_LOG_INFO, "save directory:   %s\n", retro_save_directory);

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

   libretro_serialize_size = 0;

   return 1;
}

void retro_unload_game(void)
{
   Cleanup();
}

void retro_run(void)
{
   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();

   /* Read input */
   update_input();

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
   info->need_fullpath = false;
   info->valid_extensions = "sms|bin|rom|col|gg";
   info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));

   info->timing.fps            = 60.0;
   info->timing.sample_rate    = 44100.0;
   info->geometry.base_width   = width;
   info->geometry.base_height  = height;
   info->geometry.max_width    = width;
   info->geometry.max_height   = height;
   info->geometry.aspect_ratio = 4.0 / 3.0;
}

void retro_deinit()
{
   libretro_serialize_size = 0;
   libretro_supports_bitmasks = 0;
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
   if (libretro_serialize_size == 0)
   {
      /* Something arbitrarily big.*/
      uint8_t *buffer = (uint8_t*)malloc(1000000);
      memstream_set_buffer(buffer, 1000000);

      system_save_state_mem();
      libretro_serialize_size = memstream_get_last_size();
      free(buffer);
   }

   return libretro_serialize_size;
}

bool retro_serialize(void *data, size_t size)
{
   if (size != libretro_serialize_size)
      return 0;

   memstream_set_buffer((uint8_t*)data, size);
   system_save_state_mem();
   return 1;
}

bool retro_unserialize(const void *data, size_t size)
{
   if (size != libretro_serialize_size)
      return 0;

   memstream_set_buffer((uint8_t*)data, size);
   system_load_state_mem();
   return 1;
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
