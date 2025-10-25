// Copyright (c) 2021 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

// Kernel initialization functions, called from lk_main.
void kernel_init_early(void);
void kernel_init(void);

// Kernel subsystem initialization routines called by
// kernel_init_early and kernel_init.
void thread_init_early(void);
void thread_init(void);
void mp_init(void);
void timer_init(void);
void port_init(void);
