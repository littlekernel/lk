//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <stdint.h>
#include <sys/types.h>

#include <lk/compiler.h>
#include <platform/interrupts.h>

void sun4m_intc_early_init(void);
void sun4m_intc_init(void);
void sun4m_scc_early_init(void);
void sun4m_scc_init(void);
void sun4m_timer_early_init(void);
void sun4m_timer_init(void);

handler_return sun4m_timer_irq(void *arg);

// Global interrupt sources on sun4m platform
// NOTE: not the same as the interrupt levels 0-15 on the cpu
enum {
    // VME interrupt levels, 0 - 6,
    SUN4M_VME_IRQ_0 = 0,
    // SBUS interrupt levels, 7 - 13
    SUN4M_SBUS_IRQ_0 = 7,

    // other global source
    SUN4M_KEYBOARD_IRQ = 14,
    SUN4M_SCC_IRQ = 15,
    SUN4M_ETH_IRQ = 16,
    SUN4M_AUDIO_IRQ = 17,
    SUN4M_SCSI_IRQ = 18,
    SUN4M_TIMER_GLOBAL_IRQ = 19,
    SUN4M_VIDEO_IRQ = 20,
    SUN4M_MODULE_IRQ = 21,
    SUN4M_FLOPPY_IRQ = 22,
    // 23-26 reserved
    SUN4M_VME_ASYNC_ERROR_IRQ = 27,
    SUN4M_ECC_ERROR_IRQ = 28,
    SUN4M_MS_BUFFER_ERROR_IRQ = 29,
    SUN4M_MODULE_ERROR_IRQ = 30,
    // 31 reserved

};

// when we get an irq on level 14 it's a local percpu timer interrupt.
const uint32_t SUN4M_TIMER_PERCPU_IRQ_LEVEL = 14;

// api for dealing with openprom device tree

void platform_prom_init(void *romvec);
status_t platform_prom_initialize_arena(void *romvec, uintptr_t *out_base, size_t *out_size);
int platform_prom_get_root_node(void);
int platform_prom_find_node_by_name(const char *name);
