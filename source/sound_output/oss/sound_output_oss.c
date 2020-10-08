/*
 * OSSv3/OSSv4 output sound code.
 * License : MIT
 * See docs/MIT_license.txt for more information.
*/

#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include "smsplus.h"
#include "sound_output.h"
#include "shared.h"

static int32_t oss_audio_fd = -1;
static int16_t buffer_snd[SOUND_FREQUENCY * 2];

void Sound_Init()
{
	uint32_t channels = 2;
	uint32_t format = AFMT_S16_LE;
	uint32_t tmp = SOUND_FREQUENCY;
	int32_t err_ret;
	
	option.sndrate = SOUND_FREQUENCY;
	
	oss_audio_fd = open("/dev/dsp", O_WRONLY 
	/* Probably shouldn't be used now ? */
#ifdef NONBLOCKING_AUDIO
	| O_NONBLOCK
#endif
	);
	if (oss_audio_fd < 0)
	{
		printf("Couldn't open /dev/dsp.\n");
		return;
	}
	
	err_ret = ioctl(oss_audio_fd, SNDCTL_DSP_SPEED,&tmp);
	if (err_ret == -1)
	{
		printf("Could not set sound frequency\n");
		return;
	}
	err_ret = ioctl(oss_audio_fd, SNDCTL_DSP_CHANNELS, &channels);
	if (err_ret == -1)
	{
		printf("Could not set channels\n");
		return;
	}
	err_ret = ioctl(oss_audio_fd, SNDCTL_DSP_SETFMT, &format);
	if (err_ret == -1)
	{
		printf("Could not set sound format\n");
		return;
	}
	
	return;
}

void Sound_Update()
{
	int32_t i;
	
	if (!oss_audio_fd) return;
	
	for (i = 0; i < (SOUND_FREQUENCY / snd.fps); i++) 
	{
		buffer_snd[i * 2] = snd.output[1][i] * option.soundlevel;
		buffer_snd[i * 2 + 1] = snd.output[0][i] * option.soundlevel;
	}
	write(oss_audio_fd, buffer_snd, 4 * (SOUND_FREQUENCY / snd.fps) );
}

void Sound_Close()
{
	if (oss_audio_fd >= 0)
	{
		close(oss_audio_fd);
		oss_audio_fd = -1;
	}
}
