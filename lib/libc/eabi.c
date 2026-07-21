/* Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* some cruft we have to define when using the linux toolchain */
#include <unwind.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

void *__dso_handle;

#if defined(__ARM_EABI_UNWINDER__) && __ARM_EABI_UNWINDER__

/* Our toolchain has eabi functionality built in, but they're not really used.
 * so we stub them out here. */
_Unwind_Reason_Code __aeabi_unwind_cpp_pr0(_Unwind_State state, _Unwind_Control_Block *ucbp, _Unwind_Context *context) {
    return _URC_FAILURE;
}

_Unwind_Reason_Code __aeabi_unwind_cpp_pr1(_Unwind_State state, _Unwind_Control_Block *ucbp, _Unwind_Context *context) {
    return _URC_FAILURE;
}

_Unwind_Reason_Code __aeabi_unwind_cpp_pr2(_Unwind_State state, _Unwind_Control_Block *ucbp, _Unwind_Context *context) {
    return _URC_FAILURE;
}

#endif

/* needed by some piece of EABI */
void raise(void) {
}

int __cxa_atexit(void (*destructor)(void *), void *arg, void *dso_handle) {
    return 0;
}

int __aeabi_atexit(void *arg, void (*func)(void *), void *d) {
    return __cxa_atexit(func, arg, d);
}

#if defined(__arm__)
#include <string.h>

__attribute__((weak)) void __aeabi_memcpy(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
}
__attribute__((weak)) void __aeabi_memcpy4(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
}
__attribute__((weak)) void __aeabi_memcpy8(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
}

__attribute__((weak)) void __aeabi_memclr(void *dest, size_t n) {
    memset(dest, 0, n);
}
__attribute__((weak)) void __aeabi_memclr4(void *dest, size_t n) {
    memset(dest, 0, n);
}
__attribute__((weak)) void __aeabi_memclr8(void *dest, size_t n) {
    memset(dest, 0, n);
}

__attribute__((weak)) void __aeabi_memset(void *dest, size_t n, int c) {
    memset(dest, c, n);
}
__attribute__((weak)) void __aeabi_memset4(void *dest, size_t n, int c) {
    memset(dest, c, n);
}
__attribute__((weak)) void __aeabi_memset8(void *dest, size_t n, int c) {
    memset(dest, c, n);
}
#endif

