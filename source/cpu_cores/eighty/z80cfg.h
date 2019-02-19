
#include "shared.h"

static inline int32_t cpu_readmembyte(int32_t Addr) 
{
	return cpu_readmap[(Addr) >> 10][(Addr) & 0x03FF];
}

#define readbyte_internal(Addr) cpu_readmembyte(Addr)
#define readbyte(Addr) cpu_readmembyte(Addr)

#define writebyte(Addr,Data) cpu_writemem16(Addr, Data)
#define writeport(Addr,Data,tstates) cpu_writeport16(Addr, Data)
#define readport(Addr, tstates) cpu_readport16(Addr)

#define opcode_fetch(Addr) cpu_readmembyte(Addr)


