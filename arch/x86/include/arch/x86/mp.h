/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <arch/x86.h>

// per cpu pointer pointed to by gs:
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

#define X86_PERCPU_FIELD_OFFSET(field) offsetof(x86_percpu_t, field)

// called extremely early on the boot cpu and each secondary cpu
void x86_percpu_init_early(uint cpu_num, uint apic_id);

// get the percpu struct for the current cpu
static inline x86_percpu_t *x86_get_percpu(void) {
    x86_percpu_t *percpu;
    __asm__ volatile("mov %%gs:0, %0" : "=r" (percpu));
    return percpu;
}

// get the percpu struct for a specific cpu
x86_percpu_t *x86_get_percpu_for_cpu(uint cpu_num);

#if 0
#define X86_PERCPU_GET(field) (_Generic(((x86_get_percpu())->field), \
    uint32_t: x86_read_gs_offset32, \
    uint64_t: x86_read_gs_offset64, \
    struct thread*: x86_read_gs_offset_ptr) \
    (X86_PERCPU_FIELD_OFFSET(field)))

#define X86_PERCPU_SET(field, value) (_Generic(((x86_get_percpu())->field), \
    uint32_t: x86_write_gs_offset32, \
    uint64_t: x86_write_gs_offset64, \
    struct thread*: x86_write_gs_offset_ptr) \
    (X86_PERCPU_FIELD_OFFSET(field), value))
#endif

// get the current cpu number
static inline uint x86_get_cpu_num(void) {
    return x86_read_gs_offset32(X86_PERCPU_FIELD_OFFSET(cpu_num));
}

// get the current apic id
static inline uint x86_get_apic_id(void) {
    return x86_read_gs_offset32(X86_PERCPU_FIELD_OFFSET(apic_id));
}

// get/set the current thread
struct thread;

static inline struct thread *x86_get_current_thread(void) {
    return (struct thread *)x86_read_gs_offset_ptr(X86_PERCPU_FIELD_OFFSET(current_thread));
}

static inline void x86_set_current_thread(struct thread *t) {
    x86_write_gs_offset_ptr(X86_PERCPU_FIELD_OFFSET(current_thread), t);
}