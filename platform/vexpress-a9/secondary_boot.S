/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#include <platform/vexpress-a9.h>

.section .text

/* true reset vector, to catch non boot cpus and hold them until later */
FUNCTION(platform_reset)
    mrc     p15, 0, r12, c0, c0, 5 /* read MPIDR */

    /* mask off the bottom 12 bits to test cluster number:cpu number */
    ubfx    r12, r12, #0, #12

    /* if we're cpu 0:0, continue to the usual arm reset vector */
    cmp     r12, #0
    beq     arm_reset

    /* all other cpus, trap and wait to be released */
1:
    wfe
    ldr     r12, =boot_cpu_lock
    ldr     r12, [r12]
    cmp     r12, #0
    bne     1b

    b       arm_secondary_reset

.ltorg

