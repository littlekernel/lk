#pragma once

#ifndef ASSEMBLY

#include <arch/ops.h>
#include <stdbool.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

static inline void arch_enable_ints(void) {
    CF;
}

static inline void arch_disable_ints(void) {
    CF;
}

static inline bool arch_ints_disabled(void) {
    return true;
}

static inline bool arch_in_int_handler(void) {
    return false;
}

struct arch_interrupt_saved_state {
    bool restore_irqs;
};

static inline struct arch_interrupt_saved_state arch_interrupt_save(void) {
    struct arch_interrupt_saved_state state = { .restore_irqs = false };
    return state;
}

static inline void arch_interrupt_restore(struct arch_interrupt_saved_state old_state) {
    CF;
}

__END_CDECLS

#endif // ASSEMBLY
