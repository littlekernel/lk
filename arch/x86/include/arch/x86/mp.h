/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

// per cpu pointer pointed to by a segment register on x86
typedef struct x86_percpu {
    // pointer back to ourselves so we can get a raw pointer via segment:0
    struct x86_percpu *self;

    uint cpu_num;
    uint apic_id;

    struct thread *current_thread;

    // XXX add more stuff:
    // per cpu TSS
    // per cpu doublefault/nmi stacks
} x86_percpu_t;

// called extremely early on the boot cpu and each secondary cpu
void x86_percpu_init_early(uint cpu_num, uint apic_id);