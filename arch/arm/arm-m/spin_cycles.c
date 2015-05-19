/* vim: set expandtab ts=4 sw=4 tw=100: */
/*
 * Copyright (c) 2013 Google Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <compiler.h>
#include <kernel/debug.h>

__ALIGNED(8) __NAKED
void spin_cycles(uint32_t cycles)
{
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
