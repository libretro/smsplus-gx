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

extern z80_t z80;
extern z80_t *Z80_Context;
extern int32_t z80_ICount;
extern int32_t extra_cycles;
extern int32_t z80_nmipending;
extern int32_t z80_nmi_state;
extern int32_t z80_cycle_count;
extern uint32_t z80_requested_cycles;

INLINE void z80_execute(int32_t internal_cycles)
{
	z80_ICount = (internal_cycles - (extra_cycles)) ;
	z80_requested_cycles = internal_cycles ;

	if (z80_nmipending == 1) 
	{
		z80_nmi(Z80_Context,0);
		z80_nmipending = 0;
	}
	
	while(z80_ICount > 0) 
	{
		if (sms.irq) z80_interrupt(Z80_Context, 0);
		z80_ICount-=z80_do_opcode(Z80_Context);
	}
	extra_cycles=-z80_ICount;
	z80_cycle_count += ((z80_requested_cycles - extra_cycles) - z80_ICount);
}

extern void z80_set_irq_line(uint8_t irqline, uint8_t state);
extern int32_t z80_get_elapsed_cycles();

extern uint8_t *cpu_readmap[64];
extern uint8_t *cpu_writemap[64];

extern void (*cpu_writemem16)(uint16_t address, uint8_t data);
extern void (*cpu_writeport16)(uint16_t port, uint8_t data);
extern uint8_t (*cpu_readport16)(uint16_t port);

#endif
