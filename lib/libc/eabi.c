/* Copyright (c) 2008-2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#if defined(__arm__)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unwind.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

#if defined(__ARM_EABI_UNWINDER__) && __ARM_EABI_UNWINDER__
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

extern uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem);
extern int64_t __divmoddi4(int64_t num, int64_t den, int64_t *rem);

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

#include <lk/compiler.h>

/*
 * ARM EABI divmod routines (__aeabi_uldivmod and __aeabi_ldivmod) return the
 * 64-bit quotient in (r0, r1) AND the 64-bit remainder in (r2, r3) simultaneously.
 * Standard C calling conventions cannot return 128 bits across 4 registers without
 * allocating a hidden struct return pointer in r0 (corrupting input arguments).
 * Therefore, these functions are implemented as __NAKED assembly stubs that forward
 * to __udivmoddi4 / __divmoddi4 and copy the remainder into (r2, r3) before returning.
 */
__WEAK __NAKED void __aeabi_uldivmod(void) {
    __asm__ volatile(
        "push {r4, lr}\n"
        "sub sp, sp, #12\n"
        "mov r4, sp\n"
        "push {r4}\n"
        "bl __udivmoddi4\n"
        "add sp, sp, #4\n"
        "pop {r2, r3}\n"
        "add sp, sp, #4\n"
        "pop {r4, pc}\n"
    );
}

__WEAK __NAKED void __aeabi_ldivmod(void) {
    __asm__ volatile(
        "push {r4, lr}\n"
        "sub sp, sp, #12\n"
        "mov r4, sp\n"
        "push {r4}\n"
        "bl __divmoddi4\n"
        "add sp, sp, #4\n"
        "pop {r2, r3}\n"
        "add sp, sp, #4\n"
        "pop {r4, pc}\n"
    );
}
#endif
