#include <stdint.h>
#include "CrabZ80.h"
#include "shared.h"

CrabZ80_t *cpuz80 = NULL;
int32_t z80_ICount = 0;
int32_t z80_nmipending = 0;
int32_t z80_nmi_state = 0;

uint32_t z80_cycle_count = 0;
uint32_t z80_requested_cycles = 0;

void (*cpu_writemem16)(uint16_t address, uint8_t data);
void (*cpu_writeport16)(uint16_t port, uint8_t data);
uint8_t (*cpu_readport16)(uint16_t port);

uint8_t *cpu_readmap[64];
uint8_t *cpu_writemap[64];

uint32_t elasped;

INLINE void z80_execute(int32_t internal_cycles)
{
    if(elasped >= internal_cycles)
        return;
	
	if (z80_nmipending == 1) 
	{
		CrabZ80_pulse_nmi(cpuz80);
		z80_nmipending = 0;
	}
	
	elasped = CrabZ80_execute(cpuz80, internal_cycles - elasped);
	
	printf("elasped : %d\n", elasped);
	z80_cycle_count = elasped - cpuz80->cycles;
	printf("z80_cycle_count :  %d,\n", z80_cycle_count);
}

int32_t z80_get_elapsed_cycles()
{
	//return z80_cycle_count + ((z80_requested_cycles - extra_cycles) - z80_ICount);
	//return cpuz80->cycles;
	//printf("cpuz80->cycles : %d\n", cpuz80->cycles - (z80_ICount));
	return elasped - cpuz80->cycles;
}

void z80_set_irq_line(int32_t irqline, int32_t state)
{
	if (irqline == INPUT_LINE_NMI)
	{
		if (z80_nmi_state == CLEAR_LINE && state != CLEAR_LINE) 
		{
			z80_nmipending = 1;
		}
		z80_nmi_state = state;
	}
	else
	{
		sms.irq = state;
	}
}

void CPUZ80_Init()
{
	cpuz80 = (CrabZ80_t *)malloc(sizeof(CrabZ80_t));
	CrabZ80_init(cpuz80, CRABZ80_CPU_Z80);
	CrabZ80_reset(cpuz80);
	
    CrabZ80_set_portwrite(cpuz80, cpu_writeport16);
    CrabZ80_set_portread(cpuz80, cpu_readport16);
    CrabZ80_set_readmap(cpuz80, cpu_readmap);
    CrabZ80_set_memread(cpuz80, cpu_readmembyte);
	CrabZ80_set_memwrite(cpuz80, cpu_writemem16);
	
	z80_cycle_count = 0;
}

void CPUZ80_Reset()
{
	CrabZ80_reset(cpuz80);
	z80_cycle_count = 0;
}

void CPUIRQ_Pause()
{
	CrabZ80_pulse_nmi(cpuz80);
	//z80_nmi(Z80_Context, 0);	
}
