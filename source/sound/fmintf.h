
#ifndef _FMINTF_H_
#define _FMINTF_H_

enum 
{
    SND_NULL,
    SND_EMU2413,        /* Mitsutaka Okazaki's YM2413 emulator, removed until i can switch to upstream due to unclear license */
    SND_YM2413          /* Jarek Burczynski's YM2413 emulator */
};

typedef struct 
{
    uint8_t latch;
    uint8_t reg[0x40];
} FM_Context;

/* Function prototypes */
void FM_Init(void);
void FM_Shutdown(void);
void FM_Reset(void);
void FM_Update(int16_t **buffer, uint32_t length);
void FM_Write(uint32_t offset, uint32_t data);
void FM_GetContext(uint8_t *data);
void FM_SetContext(uint8_t *data);
int FM_GetContextSize(void);
uint8_t *FM_GetContextPtr(void);
void FM_WriteReg(uint32_t reg, uint32_t data);

#endif /* _FMINTF_H_ */
