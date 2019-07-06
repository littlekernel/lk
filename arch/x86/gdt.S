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

.align 8
DATA(_gdtr_phys)
    .short _gdt_end - _gdt - 1
    .int PHYS(_gdt)

.align 8
DATA(_gdtr)
    .short _gdt_end - _gdt - 1
#if ARCH_X86_32
    .int _gdt
#elif ARCH_X86_64
    .quad _gdt
#endif

.data
.align 8
DATA(_gdt)
    .int 0
    .int 0

/* ring 0 descriptors */
.set codesel_32, . - _gdt
_code_32_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b10011010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
    .byte  0b11001111       /* G(1) D(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

.set datasel, . - _gdt
_data_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b10010010       /* P(1) DPL(00) S(1) 0 E(0) W(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

.set user_codesel_32, . - _gdt
_user_code_32_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b11111010       /* P(1) DPL(11) S(1) 1 C(0) R(1) A(0) */
    .byte  0b11001111       /* G(1) D(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */


.set user_datasel, . - _gdt
_user_data_32_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b11110010       /* P(1) DPL(11) S(1) 0 E(0) W(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

.set codesel_64, . - _gdt
_code_64_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b10011010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
    .byte  0b10101111       /* G(1) D(0) L(1) AVL(0) limit 19:16 */
    .byte  0x0              /* base 31:24 */

.set datasel_64, . - _gdt
_data_64_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b10010010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 AVL(0) limit 19:16 */
    .byte  0x0              /* base 31:24 */

.set user_codesel_64, . - _gdt
_user_code_64_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b11111010       /* P(1) DPL(11) S(1) 1 C(0) R(1) A(0) */
    .byte  0b10101111       /* G(1) D(1) L(0) AVL(0) limit 19:16 */
    .byte  0x0              /* base 31:24 */

.set user_datasel_64, . - _gdt
_user_data_64_gde:
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b11110010       /* P(1) DPL(11) S(1) 0 E(0) W(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

/* TSS descriptor */
.set tsssel, . - _gdt
_tss_gde:
.set i, 1
.rept SMP_MAX_CPUS
    .short 0                /* limit 15:00 */
    .short 0                /* base 15:00 */
    .byte  0                /* base 23:16 */
    .byte  0x89             /* P(1) DPL(00) S(0) TYPE(9) */
    .byte  0x80             /* G(1) D/B(0) L(0) AVL(0) limit 19:16 */
    .byte  0                /* base 31:24 */
    .quad  0x0000000000000000
.set i, i+1
.endr

DATA(_gdt_end)

