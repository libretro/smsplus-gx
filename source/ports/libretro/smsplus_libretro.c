#include <libretro.h>
#include <libretro_core_options.h>
#include <streams/memory_stream.h>
#include <libretro_state.h>

#ifdef _MSC_VER
#include "msvc_compat.h"
#endif

#include "shared.h"
#include "smsplus.h"
#include "ntsc/sms_ntsc.h"

static gamedata_t gdata;

t_config option;

static uint16_t *sms_bitmap = NULL;

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
static unsigned geometry_changed;

static unsigned remove_left_border;
static unsigned use_fm_sound;

static unsigned system_width;
static unsigned system_height;

/* blargg NTSC */
static unsigned use_ntsc;
static SMS_NTSC_IN_T *ntsc_screen = NULL;
static sms_ntsc_t *sms_ntsc = NULL;

#ifdef _WIN32
#define path_default_slash_c() '\\'
#else
#define path_default_slash_c() '/'
#endif

#define MAX_PORTS 2
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

#define NTSC_NONE 0
#define NTSC_MONOCHROME 1
#define NTSC_COMPOSITE 2
#define NTSC_SVIDEO 3
#define NTSC_RGB 4

extern uint32_t load_rom_mem(const char *data, size_t size);

static void filter_ntsc_init(void)
{
   sms_ntsc = (sms_ntsc_t*)malloc(sizeof(sms_ntsc_t));

   ntsc_screen = (SMS_NTSC_IN_T*)malloc(602 * 240 * sizeof(SMS_NTSC_IN_T));
   memset(ntsc_screen, 0, sizeof(602 * 240 * sizeof(SMS_NTSC_IN_T)));
}

static void filter_ntsc_cleanup(void)
{
   if (sms_ntsc)
      free(sms_ntsc);
   sms_ntsc = NULL;

   if (ntsc_screen)
      free(ntsc_screen);
   ntsc_screen = NULL;
}

static void filter_ntsc_set(void)
{
   sms_ntsc_setup_t setup;
   switch (use_ntsc)
   {
      case NTSC_MONOCHROME: setup = sms_ntsc_monochrome; break;
      case NTSC_COMPOSITE: setup = sms_ntsc_composite; break;
      case NTSC_SVIDEO: setup = sms_ntsc_svideo; break;
      case NTSC_RGB: setup = sms_ntsc_rgb; break;
      default: return;
   }

   sms_ntsc_init(sms_ntsc, &setup);
}

static void render_ntsc(void *vidbuf)
{
   int32_t output_width            = SMS_NTSC_OUT_WIDTH(system_width);
   int32_t output_height           = system_height;
   uint32_t output_pitch           = output_width << 1;

   sms_ntsc_blit(sms_ntsc, vidbuf, bitmap.pitch / sizeof(uint16_t), system_width, system_height, ntsc_screen, output_pitch);
   video_cb(ntsc_screen, output_width, output_height, output_pitch);
}

static void render_nofilter(void *vidbuf)
{
   video_cb(vidbuf, system_width, system_height, bitmap.pitch);
}

static void video_update(void)
{
   unsigned x = bitmap.viewport.x;
   static int last_width, last_height;

   system_width = bitmap.viewport.w;
   system_height = bitmap.viewport.h;

   if (sms.console != CONSOLE_GG && remove_left_border)
   {
      x = (vdp.reg[0] & 0x20) ? 8 : 0;
      system_width = VIDEO_WIDTH_SMS - x;
   }

   if (system_width != last_width || system_height != last_height)
      bitmap.viewport.changed = 1;

   last_width = system_width;
   last_height = system_height;

   if (geometry_changed)
   {
      struct retro_system_av_info info;
      retro_get_system_av_info(&info);
      environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
      geometry_changed = 0;
   }
   else if (bitmap.viewport.changed)
   {
      struct retro_system_av_info info;
      retro_get_system_av_info(&info);
      environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info);
      bitmap.viewport.changed = 0;
   }

   if (!use_ntsc)
      render_nofilter(sms_bitmap + x);
   else
      render_ntsc(sms_bitmap + x);
}

void system_manage_sram(uint8_t *sram, uint8_t slot_number, uint8_t mode)
{
   (void)sram;
   (void)slot_number;
   (void)mode;
}

static int bios_init(void)
{
   FILE *fd;
   char bios_path[1024];

   bios.rom = malloc(0x100000);
   bios.enabled = 0;

   sprintf(bios_path, "%s%s", gdata.biosdir, "bios.sms");

   fd = fopen(bios_path, "rb");
   if(fd)
   {
      uint32_t size;
      /* Seek to end of file, and get size */
      fseek(fd, 0, SEEK_END);
      size = ftell(fd);
      fseek(fd, 0, SEEK_SET);
      if (size < 0x4000) size = 0x4000;
      fread(bios.rom, size, 1, fd);
      bios.enabled = 2;
      bios.pages = size / 0x4000;
      fclose(fd);
      log_cb(RETRO_LOG_INFO, "bios loaded:      %s\n", bios_path);
   }

   if (sms.console == CONSOLE_COLECO)
   {
      sprintf(bios_path, "%s%s", gdata.biosdir, "BIOS.col");
      fd = fopen(bios_path, "rb");
      if(fd)
      {
         /* Seek to end of file, and get size */
         fread(coleco.rom, 0x2000, 1, fd);
         fclose(fd);
         log_cb(RETRO_LOG_INFO, "bios loaded:      %s\n", bios_path);
      }
      else
      {
         /* Coleco bios is required when running coleco roms */
         log_cb(RETRO_LOG_ERROR, "Cannot load required colero rom: %s\n", bios_path);
         return 0;
      }
   }
   return 1;
}

static void Cleanup(void)
{
   if (sms_bitmap)
      free(sms_bitmap);
   sms_bitmap = NULL;

   if (bios.rom)
      free(bios.rom);
   bios.rom = NULL;

   /* Deinitialize audio and video output */
   Sound_Close();

   /* Shut down */
   system_poweroff();
   system_shutdown();
}

/* Libretro implementation */

static void update_input(void)
{
#define KEYP(key) input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, key)

   unsigned port;
   unsigned startpressed = 0;

   input_poll_cb();

   input.pad[0] = 0;
   input.pad[1] = 0;
   input.system &= (sms.console == CONSOLE_GG) ? ~INPUT_START : ~INPUT_PAUSE;

   if (sms.console == CONSOLE_COLECO)
   {
      coleco.keypad[0] = 0xff;
      coleco.keypad[1] = 0xff;

      if (KEYP(RETROK_1)) coleco.keypad[0] = 1;
      else if (KEYP(RETROK_2)) coleco.keypad[0] = 2;
      else if (KEYP(RETROK_3)) coleco.keypad[0] = 3;
      else if (KEYP(RETROK_4)) coleco.keypad[0] = 4;
      else if (KEYP(RETROK_5)) coleco.keypad[0] = 5;
      else if (KEYP(RETROK_6)) coleco.keypad[0] = 6;
      else if (KEYP(RETROK_7)) coleco.keypad[0] = 7;
      else if (KEYP(RETROK_8)) coleco.keypad[0] = 8;
      else if (KEYP(RETROK_9)) coleco.keypad[0] = 9;
      else if (KEYP(RETROK_DOLLAR)) coleco.keypad[0] = 10;
      else if (KEYP(RETROK_ASTERISK)) coleco.keypad[0] = 11;
   }

   for (port = 0; port < MAX_PORTS; port++)
   {
      unsigned i;
      if (libretro_supports_bitmasks)
      {
         uint16_t ret = input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);

         for (i = 0; i < MAX_BUTTONS; i++)
            input.pad[port] |= (ret & (1 << binds[i].retro)) ? binds[i].sms : 0;

         if (!port && (ret & (1 << RETRO_DEVICE_ID_JOYPAD_START)))
            startpressed = 1;
      }
      else
      {
         for (i = 0; i < MAX_BUTTONS; i++)
            input.pad[port] |= (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, binds[i].retro)) ? binds[i].sms : 0;

         if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
            startpressed = 1;
      }
   }

   if (startpressed)
      input.system |= (sms.console == CONSOLE_GG) ? INPUT_START : INPUT_PAUSE;

   if (sms.console == CONSOLE_COLECO) input.system = 0;
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

   libretro_supports_bitmasks = 0;
   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = 1;

   check_system_specs();
   libretro_set_core_options(environ_cb);
}

void retro_reset(void)
{
   system_reset();
}

bool retro_load_game_special(unsigned id, const struct retro_game_info *info, size_t size)
{
   return false;
}

static void check_variables(bool startup)
{
   struct retro_variable var = { 0 };

   unsigned old_ntsc   = use_ntsc;
   unsigned old_border = remove_left_border;

   var.value = NULL;
   var.key   = "smsplus_hardware";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && startup)
   {
      if (strcmp(var.value, "master system") == 0)
         sms.console = CONSOLE_SMS;
      else if (strcmp(var.value, "master system II") == 0)
         sms.console = CONSOLE_SMS2;
      else if (strcmp(var.value, "game gear") == 0)
         sms.console = CONSOLE_GG;
      else if (strcmp(var.value, "game gear (sms compatibility)") == 0)
         sms.console = CONSOLE_GGMS;
      else if (strcmp(var.value, "coleco") == 0)
      {
         sms.console = CONSOLE_COLECO;
			cart.mapper = MAPPER_NONE;
      }
   }

   var.value = NULL;
   var.key   = "smsplus_region";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && startup)
   {
      if (strcmp(var.value, "ntsc-u") == 0)
      {
         sms.display = DISPLAY_NTSC;
			sms.territory = TERRITORY_EXPORT;
      }
      else if (strcmp(var.value, "pal") == 0)
      {
         sms.display = DISPLAY_PAL;
			sms.territory = TERRITORY_EXPORT;
      }
      else if (strcmp(var.value, "ntsc-j") == 0)
      {
         sms.display = DISPLAY_NTSC;
			sms.territory = TERRITORY_DOMESTIC;
      }
   }

   var.value = NULL;
   var.key   = "smsplus_fm_sound";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && startup)
   {
      if (strcmp(var.value, "disabled") == 0)
         use_fm_sound = 0;
      else
         use_fm_sound = 1;
   }

   var.value = NULL;
   var.key   = "smsplus_remove_left_border";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         remove_left_border = 1;
      else
         remove_left_border = 0;
   }

   var.value = NULL;
   var.key   = "smsplus_ntsc_filter";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "monochrome") == 0)
         use_ntsc = NTSC_MONOCHROME;
      else if (strcmp(var.value, "composite") == 0)
         use_ntsc = NTSC_COMPOSITE;
      else if (strcmp(var.value, "svideo") == 0)
         use_ntsc = NTSC_SVIDEO;
      else if (strcmp(var.value, "rgb") == 0)
         use_ntsc = NTSC_RGB;
      else
         use_ntsc = NTSC_NONE;
   }

   if (old_ntsc != use_ntsc)
   {
      geometry_changed = 1;
      filter_ntsc_set();
   }

   if (old_border != remove_left_border)
      bitmap.viewport.changed = 1;
}

bool retro_load_game(const struct retro_game_info *info)
{
   enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_RGB565;
   const char *dir = NULL;
   char retro_system_directory[256];

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

   if (!info)
      return false;

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565) && log_cb)
      log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
      sprintf(retro_system_directory, "%s", dir);

   /* Set up the bios directory */
   sprintf(gdata.biosdir, "%s%c", retro_system_directory, path_default_slash_c());

   memset(&option, 0, sizeof(option));

   option.fullscreen = 1;
   option.fm = 1;
   option.spritelimit = 1;
   option.tms_pal = 2;
   option.nosound = 0;
   option.soundlevel = 2;

   option.country = 0;
   option.console = 0;

   /* Force Colecovision mode if extension is .col */
	if (strcmp(strrchr(info->path, '.'), ".col") == 0) option.console = 6;

   /* Load ROM */
   if (!load_rom_mem((const char*)info->data, info->size))
   {
      log_cb(RETRO_LOG_ERROR, "Error: Failed to load %s.\n", info->path);
      Cleanup();
      return false;
   }

   sms_bitmap  = (uint16_t*)malloc(VIDEO_WIDTH_SMS * 240 * sizeof(uint16_t));

   /* Set parameters for internal bitmap */
   bitmap.width       = VIDEO_WIDTH_SMS;
   bitmap.height      = 240;
   bitmap.depth       = 16;
   bitmap.granularity = 2;
   bitmap.data        = (uint8_t *)sms_bitmap;
   bitmap.pitch       = VIDEO_WIDTH_SMS * sizeof(uint16_t);
   bitmap.viewport.w  = VIDEO_WIDTH_SMS;
   bitmap.viewport.h  = VIDEO_HEIGHT_SMS;
   bitmap.viewport.x  = 0x00;
   bitmap.viewport.y  = 0x00;

   filter_ntsc_init();

   check_variables(true);

   if (!bios_init()) /* This only fails when running coleco roms and coleco bios is not found */
      return false;

   /* sms.territory = settings.misc_region; */
   if (sms.console == CONSOLE_SMS || sms.console == CONSOLE_SMS2)
      sms.use_fm = use_fm_sound;

   Sound_Init();

   /* Initialize all systems and power on */
   system_poweron();

   log_cb(RETRO_LOG_INFO, "CRC : 0x%08X\n", cart.crc);

   system_width = bitmap.viewport.w;
   system_height = bitmap.viewport.h;

   libretro_serialize_size = 0;

   return true;
}

void retro_unload_game(void)
{
}

void retro_run(void)
{
   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables(false);

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
   double fps = retro_get_region() ? FPS_PAL : FPS_NTSC;
   double rate = (double)option.sndrate;

   memset(info, 0, sizeof(*info));

   info->timing.fps            = fps;
   info->timing.sample_rate    = rate;
   info->geometry.base_width   = !use_ntsc ? system_width : SMS_NTSC_OUT_WIDTH (system_width);
   info->geometry.base_height  = system_height;
   info->geometry.max_width    = !use_ntsc ? VIDEO_WIDTH_SMS : SMS_NTSC_OUT_WIDTH(VIDEO_WIDTH_SMS);
   info->geometry.max_height   = 240;
   info->geometry.aspect_ratio = 4.0 / 3.0;
}

void retro_deinit()
{
   Cleanup();

   filter_ntsc_cleanup();

   libretro_serialize_size = 0;
   libretro_supports_bitmasks = 0;
}

unsigned retro_get_region(void)
{
   return (sms.display == DISPLAY_PAL) ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned in_port, unsigned device)
{
   (void)in_port;
   (void)device;
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
