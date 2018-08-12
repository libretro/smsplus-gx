/*
    This file is part of CrabEmu.

    Copyright (C) 2005, 2006, 2007, 2008, 2009, 2015 Lawrence Sebald

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

#ifndef CRABZ80_H
#define CRABZ80_H

#include <stdio.h>
#include <stdint.h>

#ifdef IN_CRABEMU
#include "CrabEmu.h"
#else

#ifdef __cplusplus
#define CLINKAGE extern "C" {
#define ENDCLINK }
#else
#define CLINKAGE
#define ENDCLINK
#endif /* __cplusplus */

#ifndef CRABEMU_TYPEDEFS
#define CRABEMU_TYPEDEFS

#ifdef _arch_dreamcast
#include <arch/types.h>
#else

#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef uint32_t int32;
#endif /* _arch_dreamcast */
#endif /* CRABEMU_TYPEDEFS */
#endif /* IN_CRABEMU */

/******************************************************************************
                            DANGER WILL ROBINSON!

                  No User-serviceable parts below this block.

                            DANGER WILL ROBINSON!
 ******************************************************************************/
CLINKAGE

#ifndef CRABZ80_PREFIX
#define CRABZ80_PREFIX CrabZ80
#endif

/* Because any of this makes sense... */
#define CRABZ80_SYM_3(x, y) x ## _ ## y
#define CRABZ80_SYM_2(x, y) CRABZ80_SYM_3(x, y)
#define CRABZ80_SYM(x)  CRABZ80_SYM_2(CRABZ80_PREFIX, x)

#define CRABZ80_FUNC(fn)    CRABZ80_SYM(fn)
#define CRABZ80_CPU         CRABZ80_SYM(t)
#define Z80                 CRABZ80_CPU

/* CPU Types.

   These control various parts of the code to implement emulation for different
   Z80-like CPUs. */
#define CRABZ80_CPU_Z80         0
#define CRABZ80_CPU_LR35902     1
#define CRABZ80_CPU_GB          CRABZ80_CPU_LR35902

#define CRABZ80_CPU_MAX         1   /* Don't use this one... */

typedef union {
    struct {
#ifdef __BIG_ENDIAN__
        uint8 h;
        uint8 l;
#else
        uint8 l;
        uint8 h;
#endif
    } b;
    uint8 bytes[2];
    uint16 w;
} CRABZ80_SYM(reg_t);

typedef union {
    struct {
#ifdef __BIG_ENDIAN__
        uint8 l;
        uint8 h;
#else
        uint8 h;
        uint8 l;
#endif
    } b;
    uint16 w;
} CRABZ80_SYM(regAF_t);

#ifdef __BIG_ENDIAN__
#define REG8(x) cpu->regs8[(x) & 0x07]
#define OREG8(x) cpu->offset->bytes[(x) & 0x01]
#else
#define REG8(x) cpu->regs8[((x) & 0x07) ^ 0x01]
#define OREG8(x) cpu->offset->bytes[((x) & 0x01) ^ 0x01]
#endif

#define REG16(x) cpu->regs16[(x) & 0x03]

typedef struct CRABZ80_SYM(struct) {
    union {
        uint8 regs8[8];
        uint16 regs16[4];
        struct {
            CRABZ80_SYM(reg_t) bc;
            CRABZ80_SYM(reg_t) de;
            CRABZ80_SYM(reg_t) hl;
            CRABZ80_SYM(regAF_t) af;
        };
    };
    CRABZ80_SYM(reg_t) ix;
    CRABZ80_SYM(reg_t) iy;
    CRABZ80_SYM(reg_t) pc;
    CRABZ80_SYM(reg_t) sp;
    CRABZ80_SYM(reg_t) ir;
    CRABZ80_SYM(regAF_t) afp;
    CRABZ80_SYM(reg_t) bcp;
    CRABZ80_SYM(reg_t) dep;
    CRABZ80_SYM(reg_t) hlp;

    CRABZ80_SYM(reg_t) *offset;

    uint8 iff1;
    uint8 iff2;
    uint8 internal_reg;
    uint8 im;

    uint8 halt;
    uint8 ei;
    uint8 irq_pending;
    uint8 r_top;

    uint32 irq_vector;
    uint32 cycles;
    uint32 cycles_in;

    uint8 (*pread)(uint16 port);
    uint8 (*mread)(uint16 addr);

    void (*pwrite)(uint16 port, uint8 data);
    void (*mwrite)(uint16 addr, uint8 data);

    uint16 (*mread16)(uint16 addr);
    void (*mwrite16)(uint16 addr, uint16 data);

    uint32 (*exec)(struct CRABZ80_SYM(struct) *, uint32);

    uint8 *readmap[256];
} CRABZ80_SYM(t);

/* Flag definitions */
#define CRABZ80_CF      0
#define CRABZ80_NF      1
#define CRABZ80_PF      2
#define CRABZ80_XF      3
#define CRABZ80_HF      4
#define CRABZ80_YF      5
#define CRABZ80_ZF      6
#define CRABZ80_SF      7

/* Flags when emulating an LR35902. Other flag bits are all 0. */
#define CRABZ80_LR35902_CF  4
#define CRABZ80_LR35902_HF  5
#define CRABZ80_LR35902_NF  6
#define CRABZ80_LR35902_ZF  7

/* Flag setting macros */
#define CRABZ80_SET_FLAG(z80, n)    (z80)->af.b.l |= (1 << (n))
#define CRABZ80_CLEAR_FLAG(z80, n)  (z80)->af.b.l &= (~(1 << (n)))
#define CRABZ80_GET_FLAG(z80, n)    ((z80)->af.b.l >> (n)) & 1

/* Function definitions */
void CRABZ80_FUNC(init)(Z80 *cpu, int model);
void CRABZ80_FUNC(reset)(Z80 *cpu);

/* Z80 IRQs. */
void CRABZ80_FUNC(pulse_nmi)(Z80 *cpu);
void CRABZ80_FUNC(assert_irq)(Z80 *cpu, uint32 vector);
void CRABZ80_FUNC(clear_irq)(Z80 *cpu);

/* LR35902 IRQs. */
void CRABZ80_FUNC(set_irqflag_lr35902)(Z80 *cpuz80, uint8 irqs);
void CRABZ80_FUNC(set_irqen_lr35902)(Z80 *cpuz80, uint8 irqs);

uint32 CRABZ80_FUNC(execute)(Z80 *cpu, uint32 cycles);

void CRABZ80_FUNC(release_cycles)(Z80 *cpu);

void CRABZ80_FUNC(set_portread)(Z80 *cpu, uint8 (*pread)(uint16 port));
void CRABZ80_FUNC(set_memread)(Z80 *cpu, uint8 (*mread)(uint16 addr));
void CRABZ80_FUNC(set_portwrite)(Z80 *cpu,
                                 void (*pwrite)(uint16 port, uint8 data));
void CRABZ80_FUNC(set_memwrite)(Z80 *cpu,
                                void (*mwrite)(uint16 addr, uint8 data));

void CRABZ80_FUNC(set_memread16)(Z80 *cpu,
                                 uint16 (*mread16)(uint16 addr));
void CRABZ80_FUNC(set_memwrite16)(Z80 *cpu,
                                  void (*mwrite16)(uint16 addr, uint16 data));

void CRABZ80_FUNC(set_readmap)(Z80 *cpu, uint8 *readmap[256]);

ENDCLINK

#endif /* !CRABZ80_H */
