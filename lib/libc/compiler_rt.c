/* Copyright (c) 2008-2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

void *__dso_handle;

/* needed by some piece of EABI */
void raise(void) {
}

int __cxa_atexit(void (*destructor)(void *), void *arg, void *dso_handle) {
    return 0;
}

int __aeabi_atexit(void *arg, void (*func)(void *), void *d) {
    return __cxa_atexit(func, arg, d);
}
