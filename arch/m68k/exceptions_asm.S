/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.text

.weak _060_isp_unimp
_060_isp_unimp:

#if M68K_CPU >= 68010
// For 68010s and above the exception frame already has a code pushed on the stack,
// so it's easy to just vector most of the handlers into one of a few classes of handlers
// and decode the vector in C.

.align 4
_m68k_irq_vector:
    // TODO: save less state for IRQs
    moveml  %d0-%d7/%a0-%a6, %sp@-

    movel   %sp,%sp@-
    jsr     m68k_irq
    add     #4, %sp

    moveml  %sp@+, %d0-%d7/%a0-%a6
    rte

.macro exception_vector name func
.align 4
\name:
    moveml  %d0-%d7/%a0-%a6, %sp@-

    movel   %sp,%sp@-
    jsr     \func
    add     #4, %sp

    moveml  %sp@+, %d0-%d7/%a0-%a6
    rte
.endm

exception_vector _m68k_general_exception m68k_exception
exception_vector _m68k_trap_exception m68k_trap_exception

.section .text.vectab
.align 16
DATA(exc_vectors)
    // first two entries are the reset vector
    .long 0
    .long 0
    // general exceptions
    .rept (15 - 2)
    .long _m68k_general_exception
    .endr
.org (15*4)
    // uninitialized irq vector
    .long _m68k_irq_vector
.org (16*4)
    .rept (24 - 16)
    .long _m68k_general_exception
    .endr
.org (24 * 4) // offset 0x60
    // start of autovectored interrupts
    .rept (32 - 24)
    .long _m68k_irq_vector
    .endr
.org (32 * 4) // offset 0x60
    // index 32, offset 0x80
    .rept (48 - 32)
    .long _m68k_trap_exception
    .endr
.org (48 * 4)
    // start of FPU, MMU vectors
    .rept (61 - 48)
    .long _m68k_general_exception
    .endr
.org (61 * 4)
    .long _060_isp_unimp
.org (62 * 4)
    .rept (64 - 62)
    .long _m68k_general_exception
    .endr
.org (64 * 4) // offset 0x100
    // index 64, offset 0x100, end of reserved vectors
    // start of user vectors
    .rept (256 - 64)
    .long _m68k_irq_vector
    .endr
.org 4*256
END_DATA(exc_vectors)

#else

// 68000 doesn't provide vector offsets in its exception frame, so we
// need to construct our own frame that contains vector numbers and things.

.equ STUB_SHIFT, 2
.equ STUB_SIZE, (1 << STUB_SHIFT)

.equ IFRAME_REGS_BYTES, 60 // Enough for address and data register contents
.equ IFRAME_BYTES, 68 // Registers + everything else

// Stub entries point to the same common code, but reside at different offsets.
.macro gen_stub_table name common count
.align 2
\name:
    .rept \count
    bsr.w \common
    .endr
.endm

 /*
 *
 * Common handler determines vector number based on return address and stub size.
 *
 * Gets return address and vector stub, converts address to vector number, builds a frame on the stack.
 *
 * We figure out what the vector number is by checking which stub table it came from, and the slot inside that table.
 *
 * vector = basevec + ((return address - table base) / STUB_SIZE)
 *
 */
.macro common_stub name table basevec func
.align 4
\name:
    moveml %d0-%d7/%a0-%a6,%sp@-
    movel IFRAME_REGS_BYTES(%sp),%a0 // bsr return address

    // Figure out which stub entry we're dealing with. Subtract stub base from return address
    lea \table,%a1
    suba.l %a1,%a0 // Offset in to table from bsr return address
    movel %a0,%d0
    subq.l #STUB_SIZE,%d0 // Move to start of our stub entry
    lsr.l #STUB_SHIFT,%d0 // Get stub index inside the table
    add.l #\basevec,%d0 // Get vector number from table stub index

    suba.l #IFRAME_BYTES,%sp // Space on stack for our frame

    // copy saved regs into frame
    lea IFRAME_BYTES(%sp),%a0
    lea 0(%sp),%a1
    moveq #14,%d1 // Fifteen registers
1:
    // Copy all the registers, move past bsr return address
    movel %a0@+,%a1@+
    dbra %d1,1b
    addq.l #4,%a0

    // Get the original status register, place it in our frame
    movew %a0@,%d2
    movew %d2,IFRAME_REGS_BYTES(%sp) // SR

    // Same for program counter (upper and lower halves)
    movel 2(%a0),%d3
    movew %d3,IFRAME_REGS_BYTES + 4(%sp) // PC
    swap %d3
    movew %d3,IFRAME_REGS_BYTES + 2(%sp)  // PC

    // Vector table offset from the vector number
    lsl.l #2,%d0
    movew %d0,IFRAME_REGS_BYTES + 6(%sp)

    movel %sp,%sp@-
    jsr \func
    addq.l #4,%sp

    // Done with our iframe, ditch it. Restore registers
    adda.l #IFRAME_BYTES,%sp
    moveml %sp@+, %d0-%d7/%a0-%a6
    addq.l #4,%sp

    rte
.endm

// Vector table entries will be calling the stuff we generate here
gen_stub_table _m68k_general_stub_gen _m68k_general_common_stub 22   // 2..23
gen_stub_table _m68k_irq_stub_gen     _m68k_irq_common_stub      8   // 24..31
gen_stub_table _m68k_irq15_stub_gen     _m68k_irq15_common_stub      1  // 15
gen_stub_table _m68k_trap_stub_gen    _m68k_trap_common_stub     16  // 32..47
gen_stub_table _m68k_reserved_stub_gen _m68k_reserved_common_stub 16 // 48..63
gen_stub_table _m68k_user_stub_gen    _m68k_user_common_stub     192 // 64..255

common_stub _m68k_general_common_stub _m68k_general_stub_gen 2   m68k_exception
common_stub _m68k_irq_common_stub     _m68k_irq_stub_gen     24  m68k_irq
common_stub _m68k_irq15_common_stub     _m68k_irq15_stub_gen     15   m68k_irq
common_stub _m68k_trap_common_stub    _m68k_trap_stub_gen    32  m68k_trap_exception
common_stub _m68k_reserved_common_stub _m68k_reserved_stub_gen 48 m68k_exception
common_stub _m68k_user_common_stub    _m68k_user_stub_gen    64  m68k_irq

.section .text.vectab
.align 16
DATA(exc_vectors)
    // Reset vectors
    .long 0
    .long 0
    // general exceptions
    .set stub, _m68k_general_stub_gen
    .rept (15 - 2)
    .long stub
    .set stub, stub + STUB_SIZE
    .endr
.org (15 * 4)
    .set stub, _m68k_irq_stub_gen
    .long stub
    .set stub, stub + STUB_SIZE
.org (16 * 4)
    .set stub, _m68k_general_stub_gen
    .rept (24 - 16)
    .long stub
    .set stub, stub + STUB_SIZE
    .endr
.org (24 * 4)
    // irq/autovector
    .set stub, _m68k_irq_stub_gen
    .rept (32 - 24)
    .long stub
    .set stub, stub + STUB_SIZE
    .endr
.org (32 * 4)
    // traps
    .set stub, _m68k_trap_stub_gen
    .rept (48 - 32)
    .long stub
    .set stub, stub + STUB_SIZE
    .endr
.org (48 * 4)
    // FPU, MMU, reserved
    .set stub, _m68k_reserved_stub_gen
    .rept (61 - 48)
    .long stub
    .set stub, stub + STUB_SIZE
    .endr
.org (61 * 4)
    .long _060_isp_unimp
.org (62 * 4)
    .set stub, _m68k_general_stub_gen
    .rept (64 - 62)
    .long stub
    .set stub, stub + STUB_SIZE
    .endr
.org (64 * 4) // offset 0x100
    // index 64, offset 0x100, end of reserved vectors
    // start of user vectors
    .set stub, _m68k_user_stub_gen
    .rept (256 - 64)
    .long stub
    .set stub, stub + STUB_SIZE
    .endr
.org 4*256
END_DATA(exc_vectors)
#endif
