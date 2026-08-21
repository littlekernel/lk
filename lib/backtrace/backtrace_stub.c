/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* Stand in for builds with no stack walker, either because the architecture
 * has none or because the target cannot spare the frame pointers.
 *
 * Keeping the module buildable everywhere means a project fragment shared by
 * many targets can list it without knowing which of them can honour it.
 */

#include <lib/backtrace.h>

#include <stdio.h>

size_t backtrace_capture(uintptr_t pc, uintptr_t fp, uintptr_t *pcs, size_t max) {
    return 0;
}

void backtrace_print(uintptr_t pc, uintptr_t fp) {
    printf("backtrace: not available in this build\n");
}

void backtrace_print_current(void) {
    backtrace_print(0, 0);
}

void backtrace_print_thread(const struct thread *t, uintptr_t pc, uintptr_t fp) {
    backtrace_print(pc, fp);
}
