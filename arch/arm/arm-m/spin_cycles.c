/*
 * Copyright (c) 2013 Google Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/compiler.h>
#include <kernel/debug.h>

__ALIGNED(8) __NAKED
#if     (__CORTEX_M >= 0x03) || (CORTEX_SC >= 300)

void spin_cycles(uint32_t cycles) {
    asm (
        /* 4 cycles per loop, subtract out 8 cycles for the overhead of the next
         * 4 instructions, plus the call into and return from the function.
         * Then, add 3 then >> 2 to round up to the number of loop iterations.
         */
        "subs r1, %[cycles], #5\n"
        "asrs r1, r1, #2\n"
        "ble .Ldone\n"

        /* Padding to stay aligned on an 8 byte boundary, also has the added
         * advantage of normalizing the overhead (1+1+2 cycles if the branch is
         * take, or 1+1+1+1 cycles if the branch is skipped and the nop is
         * executed)
         */
        "nop\n"

        /* Main delay loop.
         * sub is 1 cycle
         * nop is 1 cycle
         * branch is 2 cycles
         */
        ".Lloop:\n"
        "subs r1, r1, #1\n"
        "nop\n"
        "bne .Lloop\n"

        ".Ldone:\n"
        "bx lr\n"
        :                       /* no output */
        : [cycles] "r" (cycles) /* input is cycles */
        : "r1"                  /* r1 gets clobbered */
    );
}

#else
/* Cortex-M0 & Cortex-M0+    */
void spin_cycles(uint32_t cycles) {
    asm (
        /* 4 cycles per loop, subtract out 8 cycles for the overhead of the next
         * 4 instructions, plus the call into and return from the function.
         * Then, add 3 then >> 2 to round up to the number of loop iterations.
         */
        "sub r1, %[cycles], #5\n"
        "asr r1, r1, #2\n"
        "cmp r1, #0\n"
        "ble .Ldone\n"

        /* Padding to stay aligned on an 8 byte boundary, also has the added
         * advantage of normalizing the overhead (1+1+2 cycles if the branch is
         * take, or 1+1+1+1 cycles if the branch is skipped and the nop is
         * executed)
         */
        "nop\n"

        /* Main delay loop.
         * sub is 1 cycle
         * nop is 1 cycle
         * branch is 2 cycles
         */
        ".Lloop:\n"
        "sub r1, r1, #1\n"
        "cmp r1,#0\n"
        "bne .Lloop\n"

        ".Ldone:\n"
        "bx lr\n"
        :                       /* no output */
        : [cycles] "r" (cycles) /* input is cycles */
        : "r1"                  /* r1 gets clobbered */
    );
}
#endif
