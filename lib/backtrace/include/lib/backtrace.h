/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <lk/compiler.h>

__BEGIN_CDECLS

/* Walking the stack needs a frame pointer in every frame, which only some
 * architectures build with. Where BACKTRACE_SUPPORTED is 0 these all still
 * exist, but produce nothing, so callers need no conditionals.
 */

/* Print a backtrace of the caller's stack. */
void backtrace_print_current(void);

/* Print a backtrace of the frame described by pc and fp, which is how a fault
 * handler reports the stack it interrupted.
 */
void backtrace_print(uintptr_t pc, uintptr_t fp);

/* Print a backtrace of a thread that is not currently running, whose pc and
 * fp come from arch_thread_get_backtrace_regs(). Frames are bounds checked
 * against that thread's stack rather than the caller's.
 */
struct thread;
void backtrace_print_thread(const struct thread *t, uintptr_t pc, uintptr_t fp);

/* Fill pcs with the return addresses of the frame described by pc and fp,
 * innermost first, and return how many were found. Mostly here so tests can
 * check the walk without parsing console output.
 */
size_t backtrace_capture(uintptr_t pc, uintptr_t fp, uintptr_t *pcs, size_t max);

__END_CDECLS
