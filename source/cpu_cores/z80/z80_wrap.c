#include <stdint.h>
#include "z80.h"
#include "shared.h"

int32_t z80_ICount = 0;
int32_t z80_nmipending = 0;
int32_t z80_nmi_state = 0;
int32_t extra_cycles = 0;

void CPUZ80_Init()
{
	z80_init(0,0,0,sms_irq_callback);
}

void CPUZ80_Reset()
{
	z80_reset();
}

void CPUIRQ_Pause()
{
	z80_set_irq_line(IRQ_LINE_NMI, ASSERT_LINE);
	z80_set_irq_line(IRQ_LINE_NMI, CLEAR_LINE);
}
