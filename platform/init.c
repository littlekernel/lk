/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <platform.h>

#include <lk/err.h>
#include <lk/debug.h>

/*
 * default implementations of these routines, if the platform code
 * chooses not to implement.
 */

__WEAK void platform_init_mmu_mappings(void) {
}

__WEAK void platform_early_init(void) {
}

__WEAK void platform_init(void) {
}

__WEAK void platform_quiesce(void) {
}


__WEAK const void *get_fdt(void) {
    return NULL;
}

