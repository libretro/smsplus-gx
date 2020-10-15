#include <libretro.h>
#include <libretro_core_options.h>
#include <streams/memory_stream.h>

#include "shared.h"
#include "smsplus.h"
#include "sound_output.h"
#include "ntsc/sms_ntsc.h"

#define SOUND_FREQUENCY 44100

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
static retro_audio_sample_batch_t audio_batch_cb;

static unsigned libretro_supports_bitmasks;
static unsigned libretro_serialize_size;
static unsigned geometry_changed;

static unsigned remove_left_border;

static unsigned system_width;
static unsigned system_height;

/* blargg NTSC */
static unsigned use_ntsc;
static SMS_NTSC_IN_T *ntsc_screen = NULL;
static sms_ntsc_t sms_ntsc;

#ifdef _WIN32
#define path_default_slash_c() '\\'
#else
#define path_default_slash_c() '/'
#endif

#define MAX_PORTS 2

#define NTSC_NONE 0
#define NTSC_MONOCHROME 1
#define NTSC_COMPOSITE 2
#define NTSC_SVIDEO 3
#define NTSC_RGB 4

#define MAX_NTSC_WIDTH (SMS_NTSC_OUT_WIDTH(VIDEO_WIDTH_SMS))

extern uint32_t load_rom_mem(const char *, size_t);

void Sound_Init(void)
{
}

void Sound_Update(int16_t* sound_buffer, unsigned long len)
{
}

void Sound_Close(void)
{
}

#ifndef MAXIM_PSG
extern sn76489_t psg_sn;
#endif

#define state_write(in_data, size) memstream_write(stream, in_data, size)
#define state_read(in_data, size)  memstream_read(stream, in_data, size)

static uint32_t system_save_state_mem(void)
{
   memstream_t *stream = memstream_open(1);

   /* Save VDP context */
   state_write(&vdp, sizeof(vdp_t));

   /* Save SMS context */
   state_write(&sms, sizeof(sms_t));

   state_write(cart.fcr, 4);

   state_write(cart.sram, 0x8000);

   /* Save Z80 context */
   state_write(Z80_Context, sizeof(z80_t));

   /* Save YM2413 context */
   state_write(YM2413_GetContextPtr(), YM2413_GetContextSize());

   state_write(FM_GetContextPtr(), FM_GetContextSize());

   /* Save SN76489 context */
   #ifdef MAXIM_PSG
   state_write(SN76489_GetContextPtr(0), SN76489_GetContextSize());
   #else
   state_write(&psg_sn, sizeof(sn76489_t));
   #endif

   memstream_close(stream);

   return 0;
}

static void system_load_state_mem(void)
{
   uint8_t *buf;
   uint32_t i;
   memstream_t *stream = memstream_open(0);

   /* Initialize everything */
   system_reset();

   /* Load VDP context */
   state_read(&vdp, sizeof(vdp_t));

   /* Load SMS context */
   state_read(&sms, sizeof(sms_t));

   /* restore video & audio settings (needed if timing changed) */
   vdp_init();
   SMSPLUS_sound_init();

   state_read(cart.fcr, 4);

   state_read(cart.sram, 0x8000);

   /* Load Z80 context */
   state_read(Z80_Context, sizeof(z80_t));

   #ifdef USE_Z80
   Z80.irq_callback = sms_irq_callback;
   #endif

   /* Load YM2413 context */
   state_read(YM2413_GetContextPtr(), YM2413_GetContextSize());

   buf = malloc(FM_GetContextSize());
   state_read(buf, FM_GetContextSize());
   FM_SetContext(buf);
   free(buf);

   /* Load SN76489 context */
   #ifdef MAXIM_PSG
   buf = malloc(SN76489_GetContextSize());
   state_read(buf, SN76489_GetContextSize());
   SN76489_SetContext(0, buf);
   free(buf);
   #else
   buf = malloc(sizeof(sn76489_t));
   state_read(buf, sizeof(sn76489_t));
   memcpy(&psg_sn, buf, sizeof(sn76489_t));
   free(buf);
   #endif

   memstream_close(stream);

   if ((sms.console != CONSOLE_COLECO) && (sms.console != CONSOLE_SG1000) && (sms.console != CONSOLE_SORDM5))
   {
      /* Cartridge by default */
      slot.rom    = cart.rom;
      slot.pages  = cart.pages;
      slot.mapper = cart.mapper;
      slot.fcr = &cart.fcr[0];

      /* Restore mapping */
      mapper_reset();
      cpu_readmap[0]  = &slot.rom[0];
      if (slot.mapper != MAPPER_KOREA_MSX)
      {
         mapper_16k_w(0,slot.fcr[0]);
         mapper_16k_w(1,slot.fcr[1]);
         mapper_16k_w(2,slot.fcr[2]);
         mapper_16k_w(3,slot.fcr[3]);
      }
      else
      {
         mapper_8k_w(0,slot.fcr[0]);
         mapper_8k_w(1,slot.fcr[1]);
         mapper_8k_w(2,slot.fcr[2]);
         mapper_8k_w(3,slot.fcr[3]);
      }
   }

   /* Force full pattern cache update */
   bg_list_index = 0x200;
   for(i = 0; i < 0x200; i++)
   {
      bg_name_list[i] = i;
      bg_name_dirty[i] = 255;
   }

   /* Restore palette */
   for(i = 0; i < PALETTE_SIZE; i++)
      palette_sync(i);
}

static void filter_ntsc_init(void)
{
   int pitch = MAX_NTSC_WIDTH * sizeof(SMS_NTSC_IN_T);

   ntsc_screen = (SMS_NTSC_IN_T*)malloc(pitch * 240);
   memset(ntsc_screen, 0, pitch * 240);
   memset(&sms_ntsc, 0, sizeof(sms_ntsc_t));
}

static void filter_ntsc_cleanup(void)
{
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

   sms_ntsc_init(&sms_ntsc, &setup);
}

static void video_update(void)
{
   static unsigned last_width, last_height;
   int x         = bitmap.viewport.x;

   system_width  = bitmap.viewport.w;
   system_height = bitmap.viewport.h;

   if (sms.console != CONSOLE_GG && remove_left_border)
   {
      x            = (vdp.reg[0] & 0x20) ? 8 : 0;
      system_width = VIDEO_WIDTH_SMS - x;
   }

   if (system_width != last_width || system_height != last_height)
      bitmap.viewport.changed = 1;

   last_width  = system_width;
   last_height = system_height;

   if (geometry_changed || bitmap.viewport.changed)
   {
      struct retro_system_av_info info;

      retro_get_system_av_info(&info);
      /* hard audio-video reset */
      if (geometry_changed)
         environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
      else
         environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info);

      geometry_changed        = 0;
      bitmap.viewport.changed = 0;
   }

   if (use_ntsc)
   {
      const uint16_t *vidbuf  = sms_bitmap + x;
      int32_t ntsc_out_width  = SMS_NTSC_OUT_WIDTH(system_width);
      uint32_t ntsc_out_pitch = ntsc_out_width << 1;

      sms_ntsc_blit(&sms_ntsc, vidbuf, bitmap.pitch / sizeof(uint16_t),
            system_width, system_height, ntsc_screen, ntsc_out_pitch);
      video_cb(ntsc_screen, ntsc_out_width, system_height, ntsc_out_pitch);
   }
   else
   {
      const uint16_t *vidbuf = sms_bitmap + x;
      video_cb(vidbuf, system_width, system_height, bitmap.pitch);
   }
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
#define JOYP(n)   (ret & (1 << (n)))
#define KEYP(key) (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, key))

   unsigned port;
   unsigned startpressed = 0;

   input_poll_cb();

   input.pad[0]     = 0;
   input.pad[1]     = 0;

   coleco.keypad[0] = ~0;
   coleco.keypad[1] = ~0;

   input.system    &= (sms.console == CONSOLE_GG) ? ~INPUT_START : ~INPUT_PAUSE;

   for (port = 0; port < MAX_PORTS; port++)
   {
      unsigned ret = 0;
      if (libretro_supports_bitmasks)
      {
         ret = input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
      }
      else
      {
         int i;
         for (i = 0; i < (1 + RETRO_DEVICE_ID_JOYPAD_R3); i++)
            ret |= input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, i);
      }

      if (JOYP(RETRO_DEVICE_ID_JOYPAD_UP))
         input.pad[port] |= INPUT_UP;
      if (JOYP(RETRO_DEVICE_ID_JOYPAD_DOWN))
         input.pad[port] |= INPUT_DOWN;
      if (JOYP(RETRO_DEVICE_ID_JOYPAD_LEFT))
         input.pad[port] |= INPUT_LEFT;
      if (JOYP(RETRO_DEVICE_ID_JOYPAD_RIGHT))
         input.pad[port] |= INPUT_RIGHT;
      if (JOYP(RETRO_DEVICE_ID_JOYPAD_B))
         input.pad[port] |= INPUT_BUTTON1;
      if (JOYP(RETRO_DEVICE_ID_JOYPAD_A))
         input.pad[port] |= INPUT_BUTTON2;

      if (sms.console == CONSOLE_COLECO)
      {
         if (KEYP(RETROK_1) || (JOYP(RETRO_DEVICE_ID_JOYPAD_X)))
            coleco.keypad[port] = 1;
         else if (KEYP(RETROK_2) || (JOYP(RETRO_DEVICE_ID_JOYPAD_Y)))
            coleco.keypad[port] = 2;
         else if (KEYP(RETROK_3) || (JOYP(RETRO_DEVICE_ID_JOYPAD_R)))
            coleco.keypad[port] = 3;
         else if (KEYP(RETROK_4) || (JOYP(RETRO_DEVICE_ID_JOYPAD_L)))
            coleco.keypad[port] = 4;
         else if (KEYP(RETROK_5) || (JOYP(RETRO_DEVICE_ID_JOYPAD_R2)))
            coleco.keypad[port] = 5;
         else if (KEYP(RETROK_6) || (JOYP(RETRO_DEVICE_ID_JOYPAD_L2)))
            coleco.keypad[port] = 6;
         else if (KEYP(RETROK_7) || (JOYP(RETRO_DEVICE_ID_JOYPAD_R3)))
            coleco.keypad[port] = 7;
         else if (KEYP(RETROK_8) || (JOYP(RETRO_DEVICE_ID_JOYPAD_L3)))
            coleco.keypad[port] = 8;
         else if (KEYP(RETROK_9))
            coleco.keypad[port] = 9;
         else if (KEYP(RETROK_DOLLAR) || (JOYP(RETRO_DEVICE_ID_JOYPAD_START)))
            coleco.keypad[port] = 10;
         else if (KEYP(RETROK_ASTERISK) || (JOYP(RETRO_DEVICE_ID_JOYPAD_SELECT)))
            coleco.keypad[port] = 11;
      }

      if (!port && (JOYP(RETRO_DEVICE_ID_JOYPAD_START)))
         startpressed = 1;
   }

   if (startpressed)
      input.system |= (sms.console == CONSOLE_GG) ? INPUT_START : INPUT_PAUSE;

   if (sms.console == CONSOLE_COLECO) input.system = 0;

#undef JOYP
#undef KEYP
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
         option.fm = 0;
      else
         option.fm = 1;
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
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Button 1" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Button 2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Coleco 2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Coleco 1" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start/Pause / Coleco #" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Coleco *" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Coleco 4" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Coleco 3" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "coleco 6" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "coleco 5" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "coleco 8" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "coleco 7" },

      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Button 1" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Button 2" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Coleco 2" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Coleco 1" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Coleco #" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Coleco *" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Coleco 4" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Coleco 3" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "coleco 6" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "coleco 5" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "coleco 8" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "coleco 7" },

      { 0, 0, 0, 0, NULL },
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

   option.fullscreen  = 1;
   option.fm          = 1;
   option.spritelimit = 1;
   option.tms_pal     = 2;
   option.nosound     = 0;
   option.soundlevel  = 2;
   option.sndrate     = SOUND_FREQUENCY;

   option.country     = 0;
   option.console     = 0;

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
      sms.use_fm = option.fm;

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
   audio_batch_cb((const int16_t*)snd.output, (size_t)snd.sample_count);
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
