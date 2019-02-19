#include <stdint.h>
#include "z80.h"
#include "shared.h"

z80_t *Z80_Context = NULL;
z80_t z80;
int32_t z80_ICount = 0;
int32_t z80_nmipending = 0;
int32_t z80_nmi_state = 0;
uint32_t z80_cycle_count = 0;
uint32_t z80_requested_cycles = 0;
int32_t extra_cycles = 0;

void (*cpu_writemem16)(uint16_t address, uint8_t data);
void (*cpu_writeport16)(uint16_t port, uint8_t data);
uint8_t (*cpu_readport16)(uint16_t port);

int32_t z80_get_elapsed_cycles()
{
	return z80_cycle_count + ((z80_requested_cycles) - z80_ICount);
}

void z80_set_irq_line(uint8_t irqline, uint8_t state)
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
	Z80_Context=&z80;
	z80_init(Z80_Context);
	sms.irq = 0;
	z80_cycle_count = 0;
}

void CPUZ80_Reset()
{
	z80_reset(Z80_Context);
	sms.irq = 0;
	z80_cycle_count = 0;
}

void CPUIRQ_Pause()
{
	z80_nmi(Z80_Context, 0);	
}
