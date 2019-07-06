/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/ops.h>

/* nothing to do to sync I & D cache on x86 */
void arch_sync_cache_range(addr_t start, size_t len) {
}
