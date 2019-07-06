/*
 * Copyright (c) 2014 Google Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.text

FUNCTION(arch_spin_trylock)
	mov	x2, x0
	mov	x1, #1
	ldaxr	x0, [x2]
	cbnz	x0, 1f
	stxr	w0, x1, [x2]
1:
	ret

FUNCTION(arch_spin_lock)
	mov	x1, #1
	sevl
1:
	wfe
	ldaxr	x2, [x0]
	cbnz	x2, 1b
	stxr	w2, x1, [x0]
	cbnz	w2, 1b
	ret

FUNCTION(arch_spin_unlock)
	stlr	xzr, [x0]
	ret
