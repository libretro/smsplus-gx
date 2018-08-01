
#ifndef _SMS_H_
#define _SMS_H_

#define TYPE_OVERSEAS   (0)
#define TYPE_DOMESTIC   (1)

/* SMS context */
typedef struct
{
    uint8_t dummy[0x2000];
    uint8_t ram[0x2000];
    uint8_t sram[0x8000];
    uint8_t fcr[4];
    uint8_t paused;
    uint8_t save;
    uint8_t country;
    uint8_t port_3F;
    uint8_t port_F2;
    uint8_t use_fm;
    uint8_t irq;
    uint8_t psg_mask;
} t_sms;

extern z80_t *Z80_Context;
extern int32_t z80_ICount;

extern uint8_t *cpu_readmap[8];
extern uint8_t *cpu_writemap[8];

/* Global data */
extern t_sms sms;

/* Function prototypes */
void sms_frame(int32_t skip_render);
void sms_init(void);
void sms_reset(void);
int  sms_irq_callback(int32_t param);
void sms_mapper_w(int32_t address, int32_t data);
void cpu_reset(void);

#endif /* _SMS_H_ */
