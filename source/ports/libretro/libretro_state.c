#include <streams/memory_stream.h>

#include "libretro_state.h"
#include "shared.h"

#ifndef MAXIM_PSG
extern sn76489_t psg_sn;
#endif

#define state_write(in_data, size) memstream_write(stream, in_data, size)
#define state_read(in_data, size) memstream_read(stream, in_data, size)

uint32_t system_save_state_mem(void)
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

void system_load_state_mem(void)
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

   /** restore video & audio settings (needed if timing changed) ***/
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
