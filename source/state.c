/******************************************************************************
 *  Sega Master System / GameGear Emulator
 *  Copyright (C) 1998-2007  Charles MacDonald
 *
 *  additionnal code by Eke-Eke (SMS Plus GX)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ******************************************************************************/

#include "shared.h"

#ifndef MAXIM_PSG
//extern sn76489_t psg_sn;
#endif

uint32_t system_save_state(FILE* fd)
{
	uint32_t i;
	
    /* Save VDP context */
    fwrite(&vdp, sizeof(vdp_t), sizeof(int8_t), fd);

    /* Save SMS context */
    fwrite(&sms, sizeof(sms_t), sizeof(int8_t), fd);

	for(i=0;i<4;i++)
		fputc(cart.fcr[i], fd);

    fwrite(cart.sram, 0x8000, sizeof(int8_t), fd);

    /* Save Z80 context */
	fwrite(Z80_Context, sizeof(z80_t), sizeof(int8_t), fd);
    
    /* Save YM2413 context */
    fwrite(FM_GetContextPtr(), FM_GetContextSize(), sizeof(int8_t), fd);

    /* Save SN76489 context */
    #ifdef MAXIM_PSG
    fwrite(SN76489_GetContextPtr(0), SN76489_GetContextSize(), sizeof(int8_t), fd);
    #else
    /*extern sn76489_t psg_sn;
    fwrite(&psg_sn, sizeof(sn76489_t), sizeof(int8_t), fd);*/
    #endif
	
	return 0;
}

void system_load_state(FILE* fd)
{
	uint16_t i;
	uint8_t *buf;
	
	/* Initialize everything */
	system_reset();
   
    /* Load VDP context */
    fread(&vdp, sizeof(vdp_t), sizeof(int8_t), fd);

    /* Load SMS context */
    fread(&sms, sizeof(sms_t), sizeof(int8_t), fd);

	/** restore video & audio settings (needed if timing changed) ***/
	vdp_init();
	SMSPLUS_sound_init();

	fread(cart.fcr, 4, sizeof(int8_t), fd);

    fread(cart.sram, 0x8000, sizeof(int8_t), fd);

    /* Load Z80 context */
    fread(Z80_Context, sizeof(z80_t), sizeof(int8_t), fd);

    /* Load YM2413 context */
    buf = malloc(FM_GetContextSize());
    if (!buf) return;
    fread(buf, FM_GetContextSize(), sizeof(int8_t), fd);
    FM_SetContext(buf);
    free(buf);

    /* Load SN76489 context */
    #ifdef MAXIM_PSG
    buf = malloc(SN76489_GetContextSize());
    if (!buf) return;
    fread(buf, SN76489_GetContextSize(), sizeof(int8_t), fd);
    SN76489_SetContext(0, buf);
    free(buf);
    #else
    /*extern sn76489_t psg_sn;
    buf = malloc(sizeof(sn76489_t));
    if (!buf) return;
    fread(buf, sizeof(sn76489_t), sizeof(int8_t), fd);
    memcpy(&psg_sn, buf, sizeof(sn76489_t));
    free(buf);*/
    #endif

	if ((sms.console != CONSOLE_COLECO) && (sms.console != CONSOLE_SG1000))
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
		bg_name_dirty[i] = 0;
	}

	/* Restore palette */
	for(i = 0; i < PALETTE_SIZE; i++)
		palette_sync(i);
}
