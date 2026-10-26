/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

#define DSB __asm__ volatile("dsb sy" ::: "memory")
#define ISB __asm__ volatile("isb" ::: "memory")

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ARM64_READ_SYSREG(reg) \
({ \
    uint64_t _val; \
    __asm__ volatile("mrs %0," TOSTRING(reg) : "=r" (_val)); \
    _val; \
})

/*
 * MSR/MRS always transfer all 64 bits of a system register and the assembler
 * only accepts an X register operand, so there is deliberately no 32 bit
 * variant of these macros: the "w" constraint selects a vector register and
 * the %w0 modifier assembles to an invalid instruction. Cast the value here so
 * that callers passing a 32 bit type still get an x register rather than
 * tripping clang's -Wasm-operand-widths.
 *
 * Note the cast sign extends a signed argument, so a negative 32 bit value
 * lands as all ones in the upper half. That half is RES0 in every register
 * written here, but pass an unsigned type if that ever stops being true.
 */
#define ARM64_WRITE_SYSREG(reg, val) \
({ \
    __asm__ volatile("msr " TOSTRING(reg) ", %0" :: "r" ((uint64_t)(val))); \
    ISB; \
})

void arm64_context_switch(vaddr_t *old_sp, vaddr_t new_sp);

uint64_t arm64_get_boot_el(void);

/* exception handling */
struct arm64_iframe_long {
    uint64_t r[30];
    uint64_t lr;
    uint64_t usp;
    uint64_t elr;
    uint64_t spsr;
};

struct arm64_iframe_short {
    uint64_t r[18];
    uint64_t lr;
    uint64_t usp;
    uint64_t elr;
    uint64_t spsr;
};

struct thread;
extern void arm64_exception_table(void);
void arm64_fpu_exception(struct arm64_iframe_long *iframe);
void arm64_fpu_save_state(struct thread *thread);

static inline void arm64_fpu_pre_context_switch(struct thread *thread) {
    uint64_t cpacr = ARM64_READ_SYSREG(cpacr_el1);
    if ((cpacr >> 20) & 3) {
        arm64_fpu_save_state(thread);
        cpacr &= ~(3 << 20);
        ARM64_WRITE_SYSREG(cpacr_el1, cpacr);
    }
}

/*
 * Hooks for exceptions taken from EL0, both overridable (the defaults are weak).
 * They run in exception context on the thread's kernel stack with interrupts
 * disabled; returning resumes user space at iframe->elr. The defaults print the
 * frame and end the thread rather than the kernel.
 */
void arm64_syscall(struct arm64_iframe_long *iframe, bool is_64bit);
void arm64_user_exception(struct arm64_iframe_long *iframe, uint32_t esr, uint64_t far);

/*
 * Weak hook for a thread about to return to EL0 from any exception with
 * THREAD_SIGNAL_KILL latched (thread_signal_kill()). Runs with interrupts
 * disabled and must not return. The default ends the thread with
 * ERR_INTERRUPTED; a user-space host overrides it to account for the thread.
 */
void arm64_user_kill(void) __NO_RETURN;

/* what the hooks do unless overridden; an override can fall back to these */
void arm64_syscall_unhandled(struct arm64_iframe_long *iframe, bool is_64bit) __NO_RETURN;
void arm64_user_exception_unhandled(struct arm64_iframe_long *iframe, uint32_t esr, uint64_t far) __NO_RETURN;

/* Local per-cpu cache flush routines.
 * These routines clean or invalidate the cache from the point of view
 * of a single cpu to the point of coherence.
 */
void arm64_local_invalidate_cache_all(void);
void arm64_local_clean_invalidate_cache_all(void);
void arm64_local_clean_cache_all(void);

/* Current Exception Level values, as contained in CurrentEL */
#define CurrentEL_EL1    (1 << 2)
#define CurrentEL_EL2    (2 << 2)

static inline bool arm64_is_kernel_in_hyp_mode(void) {
    return ARM64_READ_SYSREG(CURRENTEL) == CurrentEL_EL2;
}


__END_CDECLS

