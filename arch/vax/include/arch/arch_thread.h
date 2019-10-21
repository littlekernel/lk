/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <arch/vax.h>

void vax_context_switch(struct vax_pcb *newpcb);

struct arch_thread {
    // main process control block
    struct vax_pcb pcb;
};

