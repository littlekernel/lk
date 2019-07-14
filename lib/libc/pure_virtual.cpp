/*
 * Copyright (c) 2006 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>

extern "C" void __cxa_pure_virtual(void) {
    panic("pure virtual called\n");
}
