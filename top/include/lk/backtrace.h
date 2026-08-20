/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>

/* Print a backtrace when lib/backtrace is part of the build. Otherwise stub
 * the calls out, so that code which reports a failure can ask for one without
 * caring whether this build can produce it.
 */
#if WITH_LIB_BACKTRACE

#include <lib/backtrace.h>

#else

#include <lk/compiler.h>

__BEGIN_CDECLS

struct thread;

static inline void backtrace_print_current(void) {}
static inline void backtrace_print(uintptr_t pc, uintptr_t fp) {}
static inline void backtrace_print_thread(const struct thread *t, uintptr_t pc, uintptr_t fp) {}

__END_CDECLS

#endif
