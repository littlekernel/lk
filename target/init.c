/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <target.h>
#include <lk/compiler.h>

/*
 * default implementations of these routines, if the target code
 * chooses not to implement.
 */

__WEAK void target_early_init(void) {
}

__WEAK void target_init(void) {
}

__WEAK void target_quiesce(void) {
}

