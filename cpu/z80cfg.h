
#include "shared.h"

extern int32_t cpu_readport(int32_t port);
extern void cpu_writemem16(int32_t address, int32_t data);
extern void cpu_writeport(int32_t port, int32_t data);

static inline uint8_t cpu_readmembyte(uint16_t Addr) 
{
	return cpu_readmap[(Addr) >> 13][(Addr) & 0x1FFF];
}

#define readbyte_internal(Addr) cpu_readmembyte(Addr)
#define readbyte(Addr) cpu_readmembyte(Addr)
#define writebyte(Addr,Data) cpu_writemem16(Addr, Data)
#define writeport(Addr,Data,tstates) cpu_writeport(Addr, Data)
#define readport(Addr, tstates) cpu_readport(Addr)
#define opcode_fetch(Addr) cpu_readmembyte(Addr)


