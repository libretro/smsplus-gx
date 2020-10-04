/*
 * Pulseaudio output sound code.
 * License : MIT
 * See docs/MIT_license.txt for more information.
*/

#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <pulse/simple.h>
#include "smsplus.h"
#include "sound_output.h"
#include "shared.h"

static pa_simple *pulse_stream;
static int16_t buffer_snd[SOUND_FREQUENCY * 2];

void Sound_Init()
{
	pa_sample_spec ss;
	ss.format = PA_SAMPLE_S16LE;
	ss.channels = 2;
	ss.rate = SOUND_FREQUENCY;
	
	option.sndrate = SOUND_FREQUENCY;
	
	/* Create a new playback stream */
	pulse_stream = pa_simple_new(NULL, "smsplusgx", PA_STREAM_PLAYBACK, NULL, "smsplusgx", &ss, NULL, NULL, NULL);
	if (!pulse_stream)
	{
		fprintf(stderr, "PulseAudio: pa_simple_new() failed!\n");
	}
	return;
}

void Sound_Update()
{
	size_t i;
	size_t len = (SOUND_FREQUENCY / snd.fps);
	for (i = 0; i < len; i++) 
	{
		buffer_snd[i * 2] = snd.output[1][i] * option.soundlevel;
		buffer_snd[i * 2 + 1] = snd.output[0][i] * option.soundlevel;
	}
	if (pa_simple_write(pulse_stream, buffer_snd, len * 4, NULL) < 0)
	{
		fprintf(stderr, "PulseAudio: pa_simple_write() failed!\n");
	}
}

void Sound_Close()
{
	if(pulse_stream != NULL)
	{
		if (pa_simple_drain(pulse_stream, NULL) < 0) 
		{
			fprintf(stderr, "PulseAudio: pa_simple_drain() failed!\n");
		}
		pa_simple_free(pulse_stream);
	}
}
