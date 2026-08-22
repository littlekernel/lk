/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stddef.h>

#include <arch/arm64.h>
#include <arch/defines.h>
#include <arch/ops.h>
#include <lk/compiler.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

void arm64_mp_init(void);

// Tell the ARM64 code how many secondary cpus to expect, which
// will cause it to allocate percpu structures for them.
void arm64_set_secondary_cpu_count(int count);

struct arm64_percpu {
    uint cpu_num;
    uint64_t mpidr;
    // the kernel's struct percpu for this cpu, see arch_get_kernel_percpu()
    void *kernel_percpu;
} __CPU_ALIGN;

static inline void arm64_set_percpu(struct arm64_percpu *pc) {
    __asm__ volatile("mov x18, %0" ::"r"(pc));
}

static inline struct arm64_percpu *arm64_get_percpu(void) {
    struct arm64_percpu *pc;
    __asm__ volatile("mov %0, x18" : "=r"(pc));
    return pc;
}

// Read a field of the local arm64_percpu as a single load off x18. Going
// through arm64_get_percpu() costs a `mov xN, x18` first: the asm's output is
// opaque to the compiler, so it cannot use x18 as the base register itself.
// These are on the path of every spin_lock() in a debug build, so the extra
// instruction at a few hundred sites is worth avoiding.
#define ARM64_PERCPU_READ32(field)                                                    \
    ({                                                                                \
        uint32_t _v;                                                                  \
        __asm__ volatile("ldr %w0, [x18, %1]"                                         \
                         : "=r"(_v)                                                   \
                         : "i"(offsetof(struct arm64_percpu, field)));                \
        _v;                                                                           \
    })

#define ARM64_PERCPU_READ64(field)                                                    \
    ({                                                                                \
        uint64_t _v;                                                                  \
        __asm__ volatile("ldr %0, [x18, %1]"                                          \
                         : "=r"(_v)                                                   \
                         : "i"(offsetof(struct arm64_percpu, field)));                \
        _v;                                                                           \
    })

static inline uint arch_curr_cpu_num(void) {
    return ARM64_PERCPU_READ32(cpu_num);
}

#define ARCH_HAS_KERNEL_PERCPU_PTR 1
static inline void *arch_get_kernel_percpu(void) {
    return (void *)ARM64_PERCPU_READ64(kernel_percpu);
}

// Translate a CPU number back to the MPIDR of the CPU.
uint64_t arm64_cpu_num_to_mpidr(uint cpu_num);

__END_CDECLS
