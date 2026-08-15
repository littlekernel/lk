/*
 * Copyright (c) 2013 Google Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/compiler.h>
#include <kernel/debug.h>

/* Single implementation for every cortex-m variant. Only thumb1 instructions
 * are used here, which assemble identically on armv6-m and armv7-m, so there is
 * no reason to keep a separate thumb2 version around.
 *
 * Note the argument is read straight out of r0 per the AAPCS, since the
 * function is naked and has no compiler generated prologue.
 */
__ALIGNED(8) __NAKED
void arm_cm_spin_cycles(uint32_t cycles) {
    asm volatile (
        /* gcc hands inline asm to the assembler in divided (pre-UAL) syntax on
         * thumb1 targets, where these would have to be spelled sub/asr. Clang's
         * integrated assembler only speaks unified syntax, so ask for unified
         * explicitly. gcc restores the surrounding syntax after the block.
         */
        ".syntax unified\n"

        /* 4 cycles per loop, subtract out 8 cycles for the overhead of the next
         * 4 instructions, plus the call into and return from the function.
         * Then, add 3 then >> 2 to round up to the number of loop iterations.
         *
         * The cmp is redundant with the flags asrs already set, except for V,
         * which asrs leaves alone and ble reads.
         */
        "subs r1, r0, #5\n"
        "asrs r1, r1, #2\n"
        "cmp r1, #0\n"
        "ble .Ldone\n"

        /* Normalizes the overhead of the test above: 1+1+1+2 cycles if the
         * branch is taken, or 1+1+1+1+1 cycles if it is skipped and the nop is
         * executed.
         */
        "nop\n"

        /* Main delay loop.
         * subs is 1 cycle
         * cmp is 1 cycle
         * branch is 2 cycles
         */
        ".Lloop:\n"
        "subs r1, r1, #1\n"
        "cmp r1,#0\n"
        "bne .Lloop\n"

        ".Ldone:\n"
        "bx lr\n"
    );
}
