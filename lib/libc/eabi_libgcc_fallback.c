/* Copyright (c) 2008-2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#if defined(__arm__)
#include <stdint.h>

#include <lk/compiler.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

/*
 * ARM EABI divmod routines (__aeabi_uldivmod and __aeabi_ldivmod). libgcc
 * normally provides these directly, so this file is only added to the
 * build when no usable libgcc was found for the target toolchain/multilib
 * combination (see lib/libc/rules.mk and the LIBGCC resolution in
 * engine.mk) -- building it unconditionally would let these weak
 * definitions satisfy the reference before the linker ever consults
 * libgcc.a on the link line, silently replacing libgcc's own routines.
 *
 * __aeabi_uldivmod and __aeabi_ldivmod return the 64-bit quotient in
 * (r0, r1) AND the 64-bit remainder in (r2, r3) simultaneously. Standard C
 * calling conventions cannot return 128 bits across 4 registers without
 * allocating a hidden struct return pointer in r0 (corrupting input
 * arguments). Therefore, these functions are implemented as __NAKED
 * assembly stubs that forward to __udivmoddi4 / __divmoddi4 and copy the
 * remainder into (r2, r3) before returning.
 */

extern uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem);
extern int64_t __divmoddi4(int64_t num, int64_t den, int64_t *rem);

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
