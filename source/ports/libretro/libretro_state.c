#include <streams/memory_stream.h>

#include "libretro_state.h"
#include "shared.h"

#ifndef MAXIM_PSG
extern sn76489_t psg_sn;
#endif

static uint64_t state_write(void *in_data, size_t size, size_t n_elem, memstream_t *stream)
{
   uint64_t len = size * n_elem;
   return memstream_write(stream, in_data, len);
}

static uint64_t state_read(void *in_data, size_t size, size_t n_elem, memstream_t *stream)
{
   uint64_t len = size * n_elem;
   return memstream_read(stream, in_data, len);
}

uint32_t system_save_state_mem(void)
{
   memstream_t *fd = memstream_open(1);

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

void system_load_state_mem(void)
{
   uint8_t *buf;
   uint32_t i;
   memstream_t *fd = memstream_open(0);

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
   Z80.irq_callback = sms_irq_callback;

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

   memstream_close(fd);
}
