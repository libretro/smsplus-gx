#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <portaudio.h>
#include "smsplus.h"
#include "sound_output.h"
#include "shared.h"

PaStream *apu_stream;
static int16_t buffer_snd[SOUND_FREQUENCY * 2];

#define NONBLOCKING_AUDIO

#ifdef NONBLOCKING_AUDIO
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    /* Cast data passed through stream to our structure. */
    uint16_t *out = (uint16_t*)outputBuffer;
    int32_t i;
    (void) inputBuffer; /* Prevent unused variable warning. */
    
	for (i = 0; i < ( (SOUND_FREQUENCY / snd.fps)); i++) 
	{
		out[i * 2] = snd.output[1][i] * option.soundlevel;
		out[i * 2 + 1] = snd.output[0][i] * option.soundlevel;
	}
	
    return 0;
}
#endif

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
	//outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	err = Pa_OpenStream( &apu_stream, NULL, &outputParameters, SOUND_FREQUENCY, SOUND_SAMPLES_SIZE, paNoFlag,
	#ifdef NONBLOCKING_AUDIO
	patestCallback, NULL);
	#else
	NULL, NULL);
	#endif
	err = Pa_StartStream( apu_stream );
}

void Sound_Update()
{
	#ifndef NONBLOCKING_AUDIO
	int32_t i;
	for (i = 0; i < (4 * (SOUND_FREQUENCY / snd.fps)); i++) 
	{
		buffer_snd[i * 2] = snd.output[1][i] * option.soundlevel;
		buffer_snd[i * 2 + 1] = snd.output[0][i] * option.soundlevel;
	}
	Pa_WriteStream( apu_stream, buffer_snd, SOUND_FREQUENCY / snd.fps );
	#endif
}

void Sound_Close()
{
	int32_t err;
	err = Pa_CloseStream( apu_stream );
	err = Pa_Terminate();
}
