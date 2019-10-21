/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.text

// first level exception vectors must be 4 byte aligned
.balign 4
.globl vax_undefined_exception
vax_undefined_exception:
    halt

.balign 16
.globl vax_exception_table
vax_exception_table:
.set i, 0
.rept (0x600/4)

pushl   $i
jbr     interrupt_common
.balign 16

.set i, i + 1
.endr

FUNCTION(interrupt_common)
    halt

