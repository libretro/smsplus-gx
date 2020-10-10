#include <stdio.h>
#include <stdint.h>
#include "smsplus.h"
#include "sound_output.h"
#include "shared.h"
#include "libretro.h"

extern retro_audio_sample_batch_t audio_batch_cb;

static int16_t buffer_snd[SOUND_SAMPLES_SIZE];

void Sound_Init(void)
{
	option.sndrate = SOUND_FREQUENCY;
}

void Sound_Update(void)
{
	size_t i;
	size_t len = (SOUND_FREQUENCY / snd.fps);

	for (i = 0; i < len; i++)
	{
		buffer_snd[i * 2 + 0] = snd.output[1][i] * option.soundlevel;
		buffer_snd[i * 2 + 1] = snd.output[0][i] * option.soundlevel;
	}

	audio_batch_cb((const int16_t*)buffer_snd, len);
}

void Sound_Close(void)
{
}
