/*
    CrabEmu Modifications to this code:
      - Added typedefs and such to unglue the code from MAME's core.
      - Added mono update function for the Dreamcast port.
      - Updated to mamedev current as of September 4, 2016, backporting to C
        from C++.

    Copyright (C) 2011, 2012, 2016 Lawrence Sebald

    Modifications distributed under the same license as the rest of the code in
    this file (GPLv2 or newer).
*/

// license:GPL-2.0+
// copyright-holders:Jarek Burczynski,Ernesto Corvi
#ifndef __YM2413_H__
#define __YM2413_H__


#include <stdint.h>
/* End MAME glue... */

struct OPLL_SLOT
{
    uint32_t  ar;         /* attack rate: AR<<2           */
    uint32_t  dr;         /* decay rate:  DR<<2           */
    uint32_t  rr;         /* release rate:RR<<2           */
    uint8_t   KSR;        /* key scale rate               */
    uint8_t   ksl;        /* keyscale level               */
    uint8_t   ksr;        /* key scale rate: kcode>>KSR   */
    uint8_t   mul;        /* multiple: mul_tab[ML]        */

    /* Phase Generator */
    uint32_t  phase;      /* frequency counter            */
    uint32_t  freq;       /* frequency counter step       */
    uint8_t   fb_shift;   /* feedback shift value         */
    int32_t   op1_out[2]; /* slot1 output for feedback    */

    /* Envelope Generator */
    uint8_t   eg_type;    /* percussive/nonpercussive mode*/
    uint8_t   state;      /* phase type                   */
    uint32_t  TL;         /* total level: TL << 2         */
    int32_t   TLL;        /* adjusted now TL              */
    int32_t   volume;     /* envelope counter             */
    uint32_t  sl;         /* sustain level: sl_tab[SL]    */

    uint8_t   eg_sh_dp;   /* (dump state)                 */
    uint8_t   eg_sel_dp;  /* (dump state)                 */
    uint8_t   eg_sh_ar;   /* (attack state)               */
    uint8_t   eg_sel_ar;  /* (attack state)               */
    uint8_t   eg_sh_dr;   /* (decay state)                */
    uint8_t   eg_sel_dr;  /* (decay state)                */
    uint8_t   eg_sh_rr;   /* (release state for non-perc.)*/
    uint8_t   eg_sel_rr;  /* (release state for non-perc.)*/
    uint8_t   eg_sh_rs;   /* (release state for perc.mode)*/
    uint8_t   eg_sel_rs;  /* (release state for perc.mode)*/

    uint32_t  key;        /* 0 = KEY OFF, >0 = KEY ON     */

    /* LFO */
    uint32_t  AMmask;     /* LFO Amplitude Modulation enable mask */
    uint8_t   vib;        /* LFO Phase Modulation enable flag (active high)*/

    /* waveform select */
    uint32_t wavetable;
};

struct OPLL_CH
{
    struct OPLL_SLOT SLOT[2];
    /* phase generator state */
    uint32_t  block_fnum; /* block+fnum                   */
    uint32_t  fc;         /* Freq. freqement base         */
    uint32_t  ksl_base;   /* KeyScaleLevel Base step      */
    uint8_t   kcode;      /* key code (for key scaling)   */
    uint8_t   sus;        /* sus on/off (release speed in percussive mode)*/
};

enum {
    RATE_STEPS = (8),

    /* sinwave entries */
    SIN_BITS =       10,
    SIN_LEN  =       (1<<SIN_BITS),
    SIN_MASK =       (SIN_LEN-1),

    TL_RES_LEN =     (256),   /* 8 bits addressing (real chip) */

    /*  TL_TAB_LEN is calculated as:
     *   11 - sinus amplitude bits     (Y axis)
     *   2  - sinus sign bit           (Y axis)
     *   TL_RES_LEN - sinus resolution (X axis)
     */
    TL_TAB_LEN = (11*2*TL_RES_LEN),

    LFO_AM_TAB_ELEMENTS = 210

};

typedef struct ym2413_s {
	int32_t tl_tab[TL_TAB_LEN];

	/* sin waveform table in 'decibel' scale */
	/* two waveforms on OPLL type chips */
	uint32_t sin_tab[SIN_LEN * 2];

	struct OPLL_CH P_CH[9];         /* OPLL chips have 9 channels*/
	uint8_t   instvol_r[9];           /* instrument/volume (or volume/volume in percussive mode)*/

	uint32_t  eg_cnt;                 /* global envelope generator counter    */
	uint32_t  eg_timer;               /* global envelope generator counter works at frequency = chipclock/72 */
	uint32_t  eg_timer_add;           /* step of eg_timer                     */
	uint32_t  eg_timer_overflow;      /* envelope generator timer overlfows every 1 sample (on real chip) */

	uint8_t   rhythm;                 /* Rhythm mode                  */

	/* LFO */
	uint32_t  LFO_AM;
	int32_t   LFO_PM;
	uint32_t  lfo_am_cnt;
	uint32_t  lfo_am_inc;
	uint32_t  lfo_pm_cnt;
	uint32_t  lfo_pm_inc;

	uint32_t  noise_rng;              /* 23 bit noise shift register  */
	uint32_t  noise_p;                /* current noise 'phase'        */
	uint32_t  noise_f;                /* current noise period         */


/* instrument settings */
/*
    0-user instrument
    1-15 - fixed instruments
    16 -bass drum settings
    17,18 - other percussion instruments
*/
	uint8_t inst_tab[19][8];

	uint32_t  fn_tab[1024];           /* fnumber->increment counter   */

	uint8_t address;                  /* address register             */

	int32_t output[2];
} YM2413;

/* CrabEmu's public interface... */
YM2413 *ym2413_init(int32_t clock, int32_t rate);
void ym2413_shutdown(YM2413 *fm);
void ym2413_reset(YM2413 *fm);
void ym2413_write(YM2413 *fm, int32_t a, int32_t v);
void ym2413_update(YM2413 *fm, int16_t **buf, int32_t samples);
unsigned char ym2413_read(YM2413 *fm, int32_t a);

#endif /*__YM2413_H__*/
