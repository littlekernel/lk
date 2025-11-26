/*
 * Copyright (c) 2009 Corey Tabaka
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
FUNCTION(bzero)
    ret

/* void *memset(void *s, int c, size_t n); */
FUNCTION(memset)
    ret

