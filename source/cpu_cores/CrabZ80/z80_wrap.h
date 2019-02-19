#ifndef WRAP_H
#define WRAP_H

#include <stdint.h>

#define ASSERT_LINE 1
#define CLEAR_LINE 0
#define IRQ_LINE_NMI 127
#define INPUT_LINE_NMI 32
#define INPUT_LINE_IRQ0 0

extern void CPUZ80_Init();
extern void CPUZ80_Reset();
extern void CPUIRQ_Pause();

extern CrabZ80_t *cpuz80;
extern int32_t z80_ICount;
extern INLINE void z80_execute(int32_t cycles);
extern void z80_set_irq_line(int32_t irqline, int32_t state);
extern int32_t z80_get_elapsed_cycles();

extern uint8_t *cpu_readmap[64];
extern uint8_t *cpu_writemap[64];

extern void (*cpu_writemem16)(uint16_t address, uint8_t data);
extern void (*cpu_writeport16)(uint16_t port, uint8_t data);
extern uint8_t (*cpu_readport16)(uint16_t port);

static inline int32_t cpu_readmembyte(int32_t Addr) 
{
	return cpu_readmap[(Addr) >> 10][(Addr) & 0x03FF];
}


#endif
