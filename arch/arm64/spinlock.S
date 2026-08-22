/*
 * Copyright (c) 2014 Google Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.text

/* the contended path; the uncontended one is inlined from arch/spinlock.h */
FUNCTION(arch_spin_lock_contended)
	mov	x1, #1
	sevl
1:
	wfe
	ldaxr	x2, [x0]
	cbnz	x2, 1b
	stxr	w2, x1, [x0]
	cbnz	w2, 1b
	ret
