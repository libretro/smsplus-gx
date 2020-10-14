#include <stdio.h>
#include <stdint.h>
#include "smsplus.h"
#include "sound_output.h"
#include "shared.h"
#include "libretro.h"

extern retro_audio_sample_batch_t audio_batch_cb;

void Sound_Init(void)
{
	option.sndrate = SOUND_FREQUENCY;
}

void Sound_Update(int16_t* sound_buffer, unsigned long len)
{
	audio_batch_cb((const int16_t*)sound_buffer, (size_t)len);
}

void Sound_Close(void)
{
}
