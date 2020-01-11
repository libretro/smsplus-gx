#include "libretro_state.h"
#include "shared.h"

static uint8_t state[0x10000];
static uint32_t bufferptr;
#ifndef MAXIM_PSG
extern sn76489_t psg_sn;
#endif

typedef struct
{
   uint8_t *buf;
   size_t size;
   size_t max_size;
} memstream_t;

typedef memstream_t memstream;

static void memstream_update_pos(memstream *fp)
{
   if (fp && fp->size > fp->max_size)
      fp->max_size = fp->size;
}

static void *memstream_open(const uint8_t *name)
{
   memstream *fp;

   if (name == NULL)
      return NULL;

   fp = (memstream *)calloc(1, sizeof(*fp));
   if (fp == NULL)
      return NULL;

   fp->buf = (char *)name;
   fp->size = 0;
   fp->max_size = 0;

   return fp;
}

static void memstream_close(void *file)
{
   memstream *fp = (memstream *)file;

   if (!fp)
      return;

   free(fp);
}

static int64_t state_write(void *in_data, size_t size, size_t n_elem, void *stream)
{
   memstream *fp = (memstream *)stream;
   int64_t len = size * n_elem;

   memcpy(fp->buf + fp->size, in_data, len);
   fp->size += len;
   memstream_update_pos(fp);

   return len;
}

static int state_read(void *in_data, size_t size, size_t n_elem, void *stream)
{
   memstream *fp = (memstream *)stream;
   uint64_t len = size * n_elem;

   if (fp == NULL || in_data == NULL)
      return 0;

   memcpy(in_data, fp->buf + fp->size, len);
   fp->size += len;
   memstream_update_pos(fp);

   return 1;
}

static uint32_t system_save_state_mem_size(void)
{
   return (64 * 1024);
}

uint32_t system_save_state_mem(void* data, size_t size)
{
   void *fd = memstream_open(data);

   if (!fd)
      return 1;

   /* Save VDP context */
   state_write(&vdp, sizeof(vdp_t), sizeof(int8_t), fd);

   /* Save SMS context */
   state_write(&sms, sizeof(sms_t), sizeof(int8_t), fd);

   state_write(cart.fcr, 4, sizeof(int8_t), fd);

   state_write(cart.sram, 0x8000, sizeof(int8_t), fd);

   /* Save Z80 context */
   state_write(Z80_Context, sizeof(z80_t), sizeof(int8_t), fd);

   /* Save YM2413 context */
   state_write(FM_GetContextPtr(), FM_GetContextSize(), sizeof(int8_t), fd);

   /* Save SN76489 context */
   #ifdef MAXIM_PSG
   state_write(SN76489_GetContextPtr(0), SN76489_GetContextSize(), sizeof(int8_t), fd);
   #else
   state_write(&psg_sn, sizeof(sn76489_t), sizeof(int8_t), fd);
   #endif

   memstream_close(fd);

   return 0;
}

void system_load_state_mem(const void* data, size_t size)
{
   uint8_t *buf;
   void *fd = memstream_open(data);

   if (!fd)
      return 1;

   /* Initialize everything */
   system_reset();

   /* Load VDP context */
   state_read(&vdp, sizeof(vdp_t), sizeof(int8_t), fd);

   /* Load SMS context */
   state_read(&sms, sizeof(sms_t), sizeof(int8_t), fd);

   /** restore video & audio settings (needed if timing changed) ***/
   vdp_init();
   SMSPLUS_sound_init();

   state_read(cart.fcr, 4, sizeof(int8_t), fd);

   state_read(cart.sram, 0x8000, sizeof(int8_t), fd);

   /* Load Z80 context */
   state_read(Z80_Context, sizeof(z80_t), sizeof(int8_t), fd);

   /* Load YM2413 context */
   buf = malloc(FM_GetContextSize());
   state_read(buf, FM_GetContextSize(), sizeof(int8_t), fd);
   FM_SetContext(buf);
   free(buf);

   /* Load SN76489 context */
   #ifdef MAXIM_PSG
   buf = malloc(SN76489_GetContextSize());
   state_read(buf, SN76489_GetContextSize(), sizeof(int8_t), fd);
   SN76489_SetContext(0, buf);
   free(buf);
   #else
   buf = malloc(sizeof(sn76489_t));
   state_read(buf, sizeof(sn76489_t), sizeof(int8_t), fd);
   memcpy(&psg_sn, buf, sizeof(sn76489_t));
   free(buf);
   #endif

   memstream_close(fd);

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
   for(uint16_t i = 0; i < 0x200; i++)
   {
      bg_name_list[i] = i;
      bg_name_dirty[i] = 255;
   }

   /* Restore palette */
   for(uint32_t i = 0; i < PALETTE_SIZE; i++)
      palette_sync(i);
}
