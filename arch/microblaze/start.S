/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <asm.h>

.section ".vectors"
.globl _start
_start:
/* vector table here */
    # start vector
    bri start
    # user exception
    bri unhandled_exception
    # interrupt
    bri microblaze_irq
    # break
    bri unhandled_exception
    # hardware exception
    bri unhandled_exception

    # reserved for future
.fill (0x50 - 0x28)

.section ".text.boot"
start:
    # set the default stack
    addik   r1, r0, default_stack_top

    # set up small data pointers
    addik   r2, r0, _SDATA2_START__
    addik   r13, r0, _SDATA_START__

    # TODO: make sure irqs are disabled and any other cpu specific setup

    # zero out bss sections
    addik   r5, r0, __bss_start
    addik   r6, r0, 0
    brlid   r15, memset
    rsubik  r7, r5, __bss_end

    # arguments to main
    addik   r5, r0, 1
    addik   r6, r0, 2
    addik   r7, r0, 3
    brlid   r15, lk_main
    addik   r8, r0, 4

    # shouldn't be here
    bri     .

unhandled_exception:
    bri     .

.bss
.align 3
LOCAL_DATA(default_stack)
    .skip 4096
LOCAL_DATA(default_stack_top)

/* vim: set ts=4 sw=4 expandtab: */

