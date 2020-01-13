#include <stdio.h>
#include <stdint.h>
#include "smsplus.h"
#include "sound_output.h"
#include "shared.h"
#include "libretro.h"

extern retro_audio_sample_batch_t audio_batch_cb;

static int16_t buffer_snd[SOUND_FREQUENCY * 2];

void Sound_Init()
{
	option.sndrate = SOUND_FREQUENCY;

	return;
}

void Sound_Update()
{
	int32_t i;
	long len = (SOUND_FREQUENCY / snd.fps);

	for (i = 0; i < len; i++)
	{
		buffer_snd[i * 2] = snd.output[1][i] * option.soundlevel;
		buffer_snd[i * 2 + 1] = snd.output[0][i] * option.soundlevel;
	}

	audio_batch_cb((const int16_t*)buffer_snd, (size_t) len);
}

void Sound_Close()
{
}
