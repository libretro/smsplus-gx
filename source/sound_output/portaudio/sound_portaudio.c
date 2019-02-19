#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <portaudio.h>
#include "smsplus.h"
#include "sound_output.h"
#include "shared.h"

PaStream *apu_stream;
static int16_t buffer_snd[SOUND_FREQUENCY * 2];

void Sound_Init()
{
	int32_t err;
	err = Pa_Initialize();
	
	PaStreamParameters outputParameters;
	
	option.sndrate = SOUND_FREQUENCY;
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice) 
	{
		printf("No sound output\n");
		return;
	}

	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paInt16;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	err = Pa_OpenStream( &apu_stream, NULL, &outputParameters, SOUND_FREQUENCY, SOUND_SAMPLES_SIZE, paNoFlag, NULL, NULL);
	err = Pa_StartStream( apu_stream );
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
	Pa_WriteStream( apu_stream, buffer_snd, SOUND_FREQUENCY / snd.fps );
}

void Sound_Close()
{
	int32_t err;
	err = Pa_CloseStream( apu_stream );
	err = Pa_Terminate();
}
