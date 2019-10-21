/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <assert.h>
#include <arch/vax/mtpr.h>

// Indexes (in units of a long word) into the SCB for various vectors
// that we care about. Many are not used yet, so dont fill all of them.
enum vax_scb_index {
    foo = 0,

    scb_adaptor_base_index = 0x100/4,

    scb_device_base_index = 0x200/4,
    scb_max_index = 0x600/4
};

#define SCB_FLAG_KERNEL_STACK (0b00)
#define SCB_FLAG_INT_STACK    (0b01)

struct vax_pcb {
    uint32_t ksp;
    uint32_t esp;
    uint32_t ssp;
    uint32_t usp;
    uint32_t r[14];
    uint32_t pc;
    uint32_t psl;
    uint32_t p0br;
    uint32_t p0lr;
    uint32_t p1br;
    uint32_t p1lr;
};

static_assert(sizeof(struct vax_pcb) == 96);

