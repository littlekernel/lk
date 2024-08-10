/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <platform/debug.h>

#include <lk/compiler.h>

/* Default implementation of panic time getc/putc.
 * Just calls through to the underlying dputc/dgetc implementation
 * unless the platform overrides it.
 */
__WEAK void platform_pputc(char c) {
    return platform_dputc(c);
}

__WEAK int platform_pgetc(char *c, bool wait) {
    return platform_dgetc(c, wait);
}
