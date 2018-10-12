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
 *   Sound emulation.
 *
 ******************************************************************************/

#ifndef _SOUND_H_
#define _SOUND_H_

enum {
  STREAM_PSG_L, /* PSG left channel */
  STREAM_PSG_R, /* PSG right channel */
  STREAM_FM_MO, /* YM2413 melody channel */
  STREAM_FM_RO, /* YM2413 rhythm channel */
  STREAM_MAX    /* Total # of sound streams */
};  

/* Sound emulation structure */
typedef struct
{
  void (*mixer_callback)(int16_t **stream, int16_t **output, uint32_t length);
  int16_t *output[2];
  int16_t *stream[STREAM_MAX];
  int fm_which;
  int enabled;
  int fps;
  int buffer_size;
  int sample_count;
  int sample_rate;
  int done_so_far;
  uint32_t fm_clock;
  uint32_t psg_clock;
} snd_t;


/* Global data */
extern snd_t snd;

/* Function prototypes */
void psg_write(uint32_t data);
void psg_stereo_w(uint32_t data);
int fmunit_detect_r(void);
void fmunit_detect_w(uint32_t data);
void fmunit_write(uint32_t offset, uint32_t data);
int sound_init(void);
void sound_shutdown(void);
void sound_reset(void);
void sound_update(uint32_t line);
void sound_mixer_callback(int16_t **stream, int16_t **output, uint32_t length);

#endif /* _SOUND_H_ */
