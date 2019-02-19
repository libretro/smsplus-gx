#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ao/ao.h>
#include "smsplus.h"
#include "sound_output.h"
#include "shared.h"

static ao_device *aodevice;
static ao_sample_format aoformat;
static int16_t buffer_snd[SOUND_FREQUENCY * 2];

void Sound_Init()
{
	ao_initialize();
	memset(&aoformat, 0, sizeof(aoformat));
	
	aoformat.bits = 16;
	aoformat.channels = 2;
	aoformat.rate = SOUND_FREQUENCY;
	aoformat.byte_format = AO_FMT_LITTLE;
	option.sndrate = SOUND_FREQUENCY;
	
	aodevice = ao_open_live(ao_default_driver_id(), &aoformat, NULL); // Live output
	if (!aodevice)
		aodevice = ao_open_live(ao_driver_id("null"), &aoformat, NULL);
}

void Sound_Update()
{
	uint32_t i;
	const float volumeMultiplier = 3.0f;

	for (i = 0; i < (4 * (SOUND_FREQUENCY / snd.fps)); i++) 
	{
		buffer_snd[i * 2] = snd.output[1][i] * volumeMultiplier;
		buffer_snd[i * 2 + 1] = snd.output[0][i] * volumeMultiplier;
	}
	ao_play(aodevice, (char*)buffer_snd, SOUND_FREQUENCY / snd.fps);
}

void Sound_Close()
{
	if (aodevice)
	{
		ao_close(aodevice);
		ao_shutdown();
	}
}
