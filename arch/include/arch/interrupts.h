/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifndef ASSEMBLY

#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

// Core interrupt routines that all arches must implement inline in arch_interrupts.h.
static inline void arch_enable_ints(void);
static inline void arch_disable_ints(void);
static inline bool arch_ints_disabled(void);
static inline bool arch_in_int_handler(void);

typedef struct arch_interrupt_saved_state arch_interrupt_saved_state_t;

static inline struct arch_interrupt_saved_state arch_interrupt_save(void);
static inline void arch_interrupt_restore(struct arch_interrupt_saved_state old_state);

__END_CDECLS

#endif // !ASSEMBLY

/* include the arch specific implementations */
#include <arch/arch_interrupts.h>

