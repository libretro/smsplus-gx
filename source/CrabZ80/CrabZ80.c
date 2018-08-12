/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2007, 2008, 2009, 2011, 2012, 2015,
                  2016 Lawrence Sebald

    CrabEmu is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    CrabEmu is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CrabEmu; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "CrabZ80.h"
#include "CrabZ80_tables.h"
#include "CrabZ80_macros.h"


static Z80 *cpu = NULL;

#ifndef CRABZ80_NO_READMAP_FALLBACK

#define FETCH_ARG8(name) \
    name = cpu->readmap[cpu->pc.w >> 8] ? \
        cpu->readmap[cpu->pc.w >> 8][(uint8)cpu->pc.w] : \
        cpu->mread(cpu->pc.w);  \
    ++cpu->pc.w;

#define FETCH_ARG16(name) { \
    uint16 pc2 = cpu->pc.w + 1; \
    uint8 pc_s = cpu->pc.w >> 8; \
    uint8 pc2_s = pc2 >> 8; \
    name = (cpu->readmap[pc_s] ? \
        cpu->readmap[pc_s][(uint8)cpu->pc.w] : \
        cpu->mread(cpu->pc.w)) | \
        ((cpu->readmap[pc2_s] ? \
          cpu->readmap[pc2_s][(uint8)pc2] : \
          cpu->mread(pc2)) << 8); \
    cpu->pc.w += 2; \
}

#else

#define FETCH_ARG8(name)    \
    name = cpu->readmap[cpu->pc.w >> 8][(uint8)cpu->pc.w]; \
    ++cpu->pc.w;

#define FETCH_ARG16(name) { \
    uint16 pc2 = cpu->pc.w + 1; \
    name = cpu->readmap[cpu->pc.w >> 8][(uint8)cpu->pc.w] | \
        (cpu->readmap[pc2 >> 8][(uint8)pc2] << 8); \
    cpu->pc.w += 2; \
}

#endif

static uint32 CrabZ80_exec_z80(Z80 *cpuin, uint32 cycles);
static uint32 CrabZ80_exec_lr35902(Z80 *cpuin, uint32 cycles);

static uint8 CrabZ80_dummy_read(uint16 addr) {
    (void)addr;
    return 0;
}

static void CrabZ80_dummy_write(uint16 addr, uint8 data) {
    (void)addr;
    (void)data;
}

static uint16 CrabZ80_default_mread16(uint16 addr) {
    if(!cpu) {
#ifdef CRABZ80_DEBUG
        fprintf(stderr, "CrabZ80_default_mread16: No CPU structure set!");
#endif
        assert(0);
    }

    return cpu->mread(addr) | (cpu->mread((addr + 1) & 0x03FF) << 10);
}

static void CrabZ80_default_mwrite16(uint16 addr, uint16 data) {
    if(!cpu) {
#ifdef CRABZ80_DEBUG
        fprintf(stderr, "CrabZ80_default_mwrite16: No CPU structure set!");
#endif
        assert(0);
    }

    cpu->mwrite((addr + 1) & 0x03FF, data >> 10);
    cpu->mwrite(addr, data & 0x03FF);
}

void CRABZ80_FUNC(set_portread)(Z80 *cpuz80, uint8 (*pread)(uint16 port)) {
    if(pread == NULL)
        cpuz80->pread = CrabZ80_dummy_read;
    else
        cpuz80->pread = pread;
}

void CRABZ80_FUNC(set_memread)(Z80 *cpuz80, uint8 (*mread)(uint16 addr)) {
    if(mread == NULL)
        cpuz80->mread = CrabZ80_dummy_read;
    else
        cpuz80->mread = mread;
}

void CRABZ80_FUNC(set_portwrite)(Z80 *cpuz80,
                                 void (*pwrite)(uint16 port, uint8 data)) {
    if(pwrite == NULL)
        cpuz80->pwrite = CrabZ80_dummy_write;
    else
        cpuz80->pwrite = pwrite;
}

void CRABZ80_FUNC(set_memwrite)(Z80 *cpuz80,
                                void (*mwrite)(uint16 addr, uint8 data)) {
    if(mwrite == NULL)
        cpuz80->mwrite = CrabZ80_dummy_write;
    else
        cpuz80->mwrite = mwrite;
}

void CRABZ80_FUNC(set_memread16)(Z80 *cpuz80,
                                 uint16 (*mread16)(uint16 addr)) {
    if(mread16 == NULL)
        cpuz80->mread16 = CrabZ80_default_mread16;
    else
        cpuz80->mread16 = mread16;
}

void CRABZ80_FUNC(set_memwrite16)(Z80 *cpuz80,
                                  void (*mwrite16)(uint16 addr, uint16 data)) {
    if(mwrite16 == NULL)
        cpuz80->mwrite16 = CrabZ80_default_mwrite16;
    else
        cpuz80->mwrite16 = mwrite16;
}

void CRABZ80_FUNC(set_readmap)(Z80 *cpuz80, uint8 *readmap[64]) {
    memcpy(cpuz80->readmap, readmap, 64 * sizeof(uint8 *));
}

void CRABZ80_FUNC(init)(Z80 *cpuz80, int model) {
    cpuz80->pread = CrabZ80_dummy_read;
    cpuz80->mread = CrabZ80_dummy_read;
    cpuz80->pwrite = CrabZ80_dummy_write;
    cpuz80->mwrite = CrabZ80_dummy_write;
    cpuz80->mread16 = CrabZ80_default_mread16;
    cpuz80->mwrite16 = CrabZ80_default_mwrite16;

    memset(cpuz80->readmap, 0, 64 * sizeof(uint8 *));

    switch(model) {
        case CRABZ80_CPU_Z80:
            cpuz80->exec = &CrabZ80_exec_z80;
            break;

        default:
            fprintf(stderr, "Unknown CPU type (%d), selecting Z80.\n", model);
            cpuz80->exec = &CrabZ80_exec_z80;
            break;
    }
}

void CRABZ80_FUNC(reset)(Z80 *cpuz80) {
    cpuz80->pc.w = 0x0000;
    cpuz80->iff1 = 0;
    cpuz80->iff2 = 0;
    cpuz80->im = 0;
    cpuz80->halt = 0;
    cpuz80->r_top = 0;
    cpuz80->af.w = 0x4000;
    cpuz80->bc.w = 0x0000;
    cpuz80->de.w = 0x0000;
    cpuz80->hl.w = 0x0000;
    cpuz80->ix.w = 0xFFFF;
    cpuz80->iy.w = 0xFFFF;
    cpuz80->sp.w = 0x0000;
    cpuz80->ir.w = 0x0000;
    cpuz80->afp.w = 0x0000;
    cpuz80->bcp.w = 0x0000;
    cpuz80->dep.w = 0x0000;
    cpuz80->hlp.w = 0x0000;
    cpuz80->cycles = 0;
    cpuz80->cycles_in = 0;
    cpuz80->ei = 0;
    cpuz80->irq_pending = 0;
}

static uint32 CrabZ80_take_nmi(Z80 *cpuz80) {
    if(cpuz80->halt) {
        cpuz80->pc.w++;
        cpuz80->halt = 0;
    }

#ifndef CRABZ80_MAMEZ80_COMPAT
    ++cpuz80->ir.b.l;
#endif

    cpuz80->iff1 = 0;
    cpuz80->sp.w -= 2;
    cpuz80->mwrite16(cpuz80->sp.w, cpuz80->pc.w);
    cpuz80->pc.w = 0x0066;

    cpuz80->irq_pending &= 1;

    return 11;
}

static uint32 CrabZ80_take_irq(Z80 *cpuz80) {
    if(cpuz80->halt) {
        cpuz80->pc.w++;
        cpuz80->halt = 0;
    }

#ifndef CRABZ80_MAMEZ80_COMPAT
    ++cpuz80->ir.b.l;
#endif

    cpuz80->iff1 = cpuz80->iff2 = 0;

    switch(cpuz80->im) {
        /* This case 0 is a hack specific to the Sega Master System 2/Game Gear
           technically, we should have some sort of function to give us the
           opcode to use in mode 0, but its not implemented yet. If someone
           really wants to use CrabZ80 for something that needs mode 0,
           let me know, and I'll implement it properly. Until then,
           or when I decide to do it, it'll stay this way. */
        case 0:
        case 1:
            cpuz80->sp.w -= 2;
            cpuz80->mwrite16(cpuz80->sp.w, cpuz80->pc.w);
            cpuz80->pc.w = 0x0038;
            return 13;

        case 2:
        {
            uint16 tmp = (cpuz80->ir.b.h << 8) + (cpuz80->irq_vector & 0xFF);
            cpuz80->sp.w -= 2;
            cpuz80->mwrite16(cpuz80->sp.w, cpuz80->pc.w);
            cpuz80->pc.w = cpuz80->mread16(tmp);
            return 19;
        }

        default:
#ifdef CRABZ80_DEBUG
            fprintf(stderr, "Invalid interrupt mode %d, report this error\n",
                    cpuz80->im);
            assert(0);
#endif
            return 0;
    }
}

void CRABZ80_FUNC(pulse_nmi)(Z80 *cpuz80) {
    cpuz80->irq_pending |= 2;
}

void CRABZ80_FUNC(assert_irq)(Z80 *cpuz80, uint32 vector) {
    cpuz80->irq_pending |= 1;
    cpuz80->irq_vector = vector;
}

void CRABZ80_FUNC(clear_irq)(Z80 *cpuz80) {
    cpuz80->irq_pending &= 2;
}

void CRABZ80_FUNC(set_irqflag_lr35902)(Z80 *cpuz80, uint8 irqs) {
    cpuz80->irq_pending = irqs;
}

void CRABZ80_FUNC(set_irqen_lr35902)(Z80 *cpuz80, uint8 irqs) {
    /* Reuse IFF2, since it doesn't exist on the LR35902. */
    cpuz80->iff2 = irqs;
}

void CRABZ80_FUNC(release_cycles)(Z80 *cpuz80) {
    cpuz80->cycles_in = 0;
}

uint32 CRABZ80_FUNC(execute)(Z80 *cpuin, uint32 cycles) {
    return cpuin->exec(cpuin, cycles);
}

static uint32 CrabZ80_exec_z80(Z80 *cpuin, uint32 cycles) {
    register uint32 cycles_done = 0;
    CrabZ80_t *oldcpu = NULL;
    uint32 oldcyclesin = 0, oldcycles = 0;

    if(cpu) {
        oldcyclesin = cpu->cycles_in;
        oldcycles = cpu->cycles;
        oldcpu = cpu;
    }

    cpu = cpuin;

    cycles_done = 0;
    cpuin->cycles_in = cycles;
    cpuin->cycles = 0;

    while(cycles_done < cpuin->cycles_in) {
        if(cpuin->irq_pending & 2) {
            cycles_done += CrabZ80_take_nmi(cpuin);
        }
        else if(cpuin->irq_pending && !cpuin->ei && cpuin->iff1) {
            cycles_done += CrabZ80_take_irq(cpuin);
        }

        cpuin->ei = 0;
        {
            uint8 inst;
            FETCH_ARG8(inst);
            ++cpuin->ir.b.l;
#define INSIDE_CRABZ80_EXECUTE
#include "CrabZ80ops.h"
#undef INSIDE_CRABZ80_EXECUTE
        }

out:
        cpuin->cycles = cycles_done;
    }

    cpu = oldcpu;

    if(cpu) {
        cpu->cycles = oldcycles;
        cpu->cycles_in = oldcyclesin;
    }

    return cycles_done;
}
