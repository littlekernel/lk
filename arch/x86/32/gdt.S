/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/x86/descriptor.h>

#define PHYS_LOAD_ADDRESS (MEMBASE + KERNEL_LOAD_OFFSET)
#define PHYS_ADDR_DELTA (KERNEL_BASE + KERNEL_LOAD_OFFSET - PHYS_LOAD_ADDRESS)
#define PHYS(x) ((x) - PHYS_ADDR_DELTA)

.section .rodata

.balign 8
DATA(_gdtr_phys)
    .short _gdt_end - _gdt - 1
    .int PHYS(_gdt)
END_DATA(_gdtr_phys)

.balign 8
DATA(_gdtr)
    .short _gdt_end - _gdt - 1
    .int _gdt
END_DATA(_gdtr)


// 32bit GDT, laid out in a specific way due to requirements by the SYSENTER/SYSEXIT and
// SYSCALL/SYSRET instructions:
//
// CODE32   <- IA32_SYSENTER_CS, IA32_STAR.SYSCALL_CS
// DATA
// UCODE32  <- IA32_STAR.SYSRET_CS
// UDATA
.data
.balign 8
DATA(_gdt)
    .int 0
    .int 0

/* ring 0 code 32bit (for bootstrapping into 64bit) */
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b10011010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
    .byte  0b11001111       /* G(1) D(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

/* ring 0 data 32bit */
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b10010010       /* P(1) DPL(00) S(1) 0 E(0) W(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

/* ring 3 code 32bit */
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b11111010       /* P(1) DPL(11) S(1) 1 C(0) R(1) A(0) */
    .byte  0b11001111       /* G(1) D(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

/* ring 3 data 32bit */
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b11110010       /* P(1) DPL(11) S(1) 0 E(0) W(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

/* per-cpu TSS descriptor */
.set i, 1
.rept SMP_MAX_CPUS
    .short 0                /* limit 15:00 */
    .short 0                /* base 15:00 */
    .byte  0                /* base 23:16 */
    .byte  0x89             /* P(1) DPL(00) S(0) TYPE(9) */
    .byte  0x80             /* G(1) D/B(0) L(0) AVL(0) limit 19:16 */
    .byte  0                /* base 31:24 */
.set i, i+1
.endr

/* per-cpu GS descriptor for x86-32 */
.set i, 1
.rept SMP_MAX_CPUS
    .int   0               /* filled in by C code later */
    .int   0
.set i, i+1
.endr

END_DATA(_gdt)

DATA(_gdt_end)

