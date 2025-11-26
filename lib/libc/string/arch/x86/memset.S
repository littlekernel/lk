/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
 /* 
* Copyright 2016 The Fuchsia Authors
*
* Use of this source code is governed by a MIT-style
* license that can be found in the LICENSE file or at
* https://opensource.org/licenses/MIT
*/
#include <lk/asm.h>

/* TODO: */

.text
.align 2

/* void bzero(void *s, size_t n); */
/*
FUNCTION(bzero)
    ret
END_FUNCTION(bzero)
*/

/* void *memset(void *s, int c, size_t n); */
/* memset implementation relying on Intel's Enhanced REP STOSB optimization */
FUNCTION(memset_erms)
    mov %rdi, %r11

    mov %sil, %al
    mov %rdx, %rcx
    rep stosb /* while (rcx-- > 0) *rdi++ = al; */

    mov %r11, %rax
    ret
END_FUNCTION(memset_erms)

/* memset implementation that sets 8 bytes at a time when possible */
FUNCTION(memset_quad)
    mov %rdi, %r11

    /* Create an 8-byte copy of the pattern */
    mov %rdx, %rcx
    movzx %sil, %rax
    movq $0x0101010101010101, %r10
    mul %r10
    mov %rcx, %rdx

    /* Copy all of the 8 byte chunks we can */
    shr $3, %rcx
    rep stosq /* while (rcx-- > 0) { *rdi++ = rax; } rdi is uint64_t* */

    // Copy the rest
    mov %rdx, %rcx
    and $0x7, %rcx
    rep stosb

    mov %r11, %rax
    ret
END_FUNCTION(memset_quad)

FUNCTION(memset)
    jmp memset_erms
END_FUNCTION(memset)

