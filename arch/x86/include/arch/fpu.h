/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <kernel/thread.h>
#include <lk/compiler.h>
#include <stdbool.h>

void x86_fpu_early_init(void);
void x86_fpu_init(void);
void x86_fpu_early_init_percpu(void);
void fpu_init_thread_states(thread_t *t);
void fpu_context_switch(thread_t *old_thread, thread_t *new_thread);
void fpu_dev_na_handler(void);

#if X86_LEGACY
// CPU=legacy x87 support, see arch/x86/x87_legacy.c. No per thread x87 state.
void x86_x87_legacy_init(void);
// a 386 with a 387 reports x87 errors on IRQ13 rather than #MF
bool x86_x87_errors_on_irq13(void);
// body of the platform's IRQ13 handler: releases the coprocessor and panics
__NO_RETURN void x86_x87_irq13_error(void);
#endif

/* End of file */
