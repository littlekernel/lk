/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/*
 * Fallback implementations of the __atomic_* library routines the compiler
 * emits when it cannot inline an atomic operation.
 *
 * Some architectures have no atomic read-modify-write instruction at all
 * (armv6-m has no ldrex/strex, for example), so both gcc and clang fall back to
 * a call into libatomic. No bare metal toolchain ships libatomic, so those
 * calls end up as undefined symbols at link time. This module fills them in.
 *
 * Signatures come from the library interface described at
 * https://gcc.gnu.org/wiki/Atomic/GCCMM/LIbrary, which clang implements as
 * well. Note the library form of compare_exchange has no 'weak' argument.
 *
 * Only architectures that need it should depend on this module. It is not a
 * transparent replacement for real atomics: the read-modify-write routines are
 * made atomic by disabling interrupts, which only serializes against other code
 * on this cpu.
 */

#include <arch/interrupts.h>

#include <stdbool.h>
#include <stdint.h>

#if WITH_SMP
#error "lib/atomic_fallback implements atomics by disabling interrupts, which " \
       "does not serialize against other cpus. It cannot be used on an SMP build."
#endif

#if !defined(__clang__)
/*
 * gcc's internal declaration of __atomic_compare_exchange_N carries an extra
 * 'weak' argument that is not actually passed at the call site: both compilers
 * emit (ptr, expected, desired, success order, failure order), which is also
 * what libatomic implements. Match the real calling convention and silence the
 * complaint about gcc's declaration.
 */
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif

/*
 * Plain aligned loads and stores are already atomic on the architectures that
 * need this module, so those routines only have to supply the ordering. Full
 * barriers on both sides of the access covers every memory model, and the
 * relaxed case (the common one) costs nothing but a compiler barrier.
 */
static inline void atomic_fallback_fence(int memorder) {
    if (memorder == __ATOMIC_RELAXED) {
        __asm__ volatile("" ::: "memory");
    } else {
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
    }
}

/*
 * The read-modify-write routines need real mutual exclusion. Interrupts off is
 * enough on a single cpu, and doubles as a full barrier, so the memory order
 * argument does not need any further handling.
 */
#define ATOMIC_FALLBACK_RMW(name, type, expr) \
    type name(volatile void *ptr, type val, int memorder); \
    type name(volatile void *ptr, type val, int memorder) { \
        volatile type *p = (volatile type *)ptr; \
        arch_interrupt_saved_state_t state = arch_interrupt_save(); \
        type old = *p; \
        *p = (type)(expr); \
        arch_interrupt_restore(state); \
        return old; \
    }

#define ATOMIC_FALLBACK_WIDTH(width, type) \
    type __atomic_load_##width(const volatile void *ptr, int memorder); \
    type __atomic_load_##width(const volatile void *ptr, int memorder) { \
        atomic_fallback_fence(memorder); \
        type val = *(const volatile type *)ptr; \
        atomic_fallback_fence(memorder); \
        return val; \
    } \
    \
    void __atomic_store_##width(volatile void *ptr, type val, int memorder); \
    void __atomic_store_##width(volatile void *ptr, type val, int memorder) { \
        atomic_fallback_fence(memorder); \
        *(volatile type *)ptr = val; \
        atomic_fallback_fence(memorder); \
    } \
    \
    bool __atomic_compare_exchange_##width(volatile void *ptr, void *expected, \
                                           type desired, int success, int failure); \
    bool __atomic_compare_exchange_##width(volatile void *ptr, void *expected, \
                                           type desired, int success, int failure) { \
        volatile type *p = (volatile type *)ptr; \
        arch_interrupt_saved_state_t state = arch_interrupt_save(); \
        type old = *p; \
        bool match = (old == *(type *)expected); \
        if (match) { \
            *p = desired; \
        } else { \
            *(type *)expected = old; \
        } \
        arch_interrupt_restore(state); \
        return match; \
    } \
    \
    ATOMIC_FALLBACK_RMW(__atomic_exchange_##width, type, val) \
    ATOMIC_FALLBACK_RMW(__atomic_fetch_add_##width, type, old + val) \
    ATOMIC_FALLBACK_RMW(__atomic_fetch_sub_##width, type, old - val) \
    ATOMIC_FALLBACK_RMW(__atomic_fetch_and_##width, type, old & val) \
    ATOMIC_FALLBACK_RMW(__atomic_fetch_or_##width, type, old | val) \
    ATOMIC_FALLBACK_RMW(__atomic_fetch_xor_##width, type, old ^ val) \
    ATOMIC_FALLBACK_RMW(__atomic_fetch_nand_##width, type, ~(old & val))

ATOMIC_FALLBACK_WIDTH(1, uint8_t)
ATOMIC_FALLBACK_WIDTH(2, uint16_t)
ATOMIC_FALLBACK_WIDTH(4, uint32_t)

#undef ATOMIC_FALLBACK_WIDTH
#undef ATOMIC_FALLBACK_RMW
