/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* Stand in for architectures with no stack walker.
 *
 * Keeping the module buildable everywhere means a project fragment shared by
 * many targets can list it without knowing which architectures can honour it.
 */

#include <lib/backtrace.h>

#include <stdio.h>

size_t backtrace_capture(uintptr_t pc, uintptr_t fp, uintptr_t *pcs, size_t max) {
    return 0;
}

void backtrace_print(uintptr_t pc, uintptr_t fp) {
    printf("backtrace: not supported on this architecture\n");
}

void backtrace_print_current(void) {
    backtrace_print(0, 0);
}
