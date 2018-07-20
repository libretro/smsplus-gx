#ifndef SN76496_H
#define SN76496_H

#define MAX_76496 4

typedef struct
{
	int32_t Channel;
	int32_t SampleRate;
	uint32_t  UpdateStep;
    int32_t VolTable[16];
    int32_t Register[8];
    int32_t LastRegister;
    int32_t Volume[4];
    uint32_t  RNG;
    int32_t NoiseFB;
	int32_t Period[4];
	int32_t Count[4];
	int32_t Output[4];
}t_SN76496;

extern t_SN76496 sn[MAX_76496];

void SN76496Write(int32_t chip,int32_t data);
void SN76496Update(int32_t chip, int16_t *buffer[2],int32_t length,uint8_t mask);
void SN76496_set_clock(int32_t chip,int32_t clock);
void SN76496_set_gain(int32_t chip,int32_t gain);
int32_t SN76496_init(int32_t chip,int32_t clock,int32_t volume,int32_t sample_rate);

#endif
