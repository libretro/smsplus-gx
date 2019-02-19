#ifndef WRAP_H
#define WRAP_H

#define ASSERT_LINE 1
#define CLEAR_LINE 0
#define IRQ_LINE_NMI 127
#define INPUT_LINE_NMI 32
#define INPUT_LINE_IRQ0 0

extern void CPUZ80_Init();
extern void CPUZ80_Reset();
extern void CPUIRQ_Pause();

extern int32_t z80_ICount;
extern int32_t extra_cycles;
extern int32_t z80_nmipending;
extern int32_t z80_nmi_state;

extern void z80_set_irq_line(int irqline, int state);
extern int32_t z80_get_elapsed_cycles();

extern uint8_t *cpu_readmap[64];
extern uint8_t *cpu_writemap[64];

#endif
