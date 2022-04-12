/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/m68k.h>
#include <inttypes.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <target.h>

#define LOCAL_TRACE 0

typedef struct m68k_iframe {
    uint32_t d[8];
    uint32_t a[7];
    uint16_t sr;
    uint16_t pc_high;
    uint16_t pc_low;
    uint16_t format : 4;
    uint16_t vector_offset : 12;
} m68k_iframe_t;

void dump_iframe(const m68k_iframe_t *iframe) {
    printf("pc 0x%08x sr 0x%04x format %#x vector %#x\n", iframe->pc_low | iframe->pc_high << 16, iframe->sr,
            iframe->format, iframe->vector_offset / 4);
    printf("d0 0x%08x d1 0x%08x d2 0x%08x d3 0x%08x\n", iframe->d[0], iframe->d[1], iframe->d[2], iframe->d[3]);
    printf("d4 0x%08x d5 0x%08x d6 0x%08x d7 0x%08x\n", iframe->d[4], iframe->d[5], iframe->d[6], iframe->d[7]);
    printf("a0 0x%08x a1 0x%08x a2 0x%08x a3 0x%08x\n", iframe->a[0], iframe->a[1], iframe->a[2], iframe->a[3]);
    printf("a4 0x%08x a5 0x%08x a6 0x%08x\n", iframe->a[4], iframe->a[5], iframe->a[6]);
}

void m68k_exception(m68k_iframe_t *frame) {
    uint8_t code = frame->vector_offset / 4;

    LTRACEF("frame %p, code %#hhx\n", frame, code);

    dump_iframe(frame);

    printf("more stack:\n");
    hexdump8(frame, 256);

    panic("unimplemented exception %#hhx\n", code);
}

void m68k_trap_exception(m68k_iframe_t *frame) {
    panic("unhandled trap exception!\n");
}

void m68k_null_trap(m68k_iframe_t *frame) {
    panic("unhandled null trap exception!");
}

// defined in platform interrupt controller
extern enum handler_return m68k_platform_irq(uint8_t irq);

void m68k_irq(m68k_iframe_t *frame) {
    uint8_t code = frame->vector_offset / 4;

    LTRACEF("frame %p, code %#hhx\n", frame, code);

    THREAD_STATS_INC(interrupts);

    if (unlikely(code == 0)) {
        // spurious interrupt
        return;
    }

    target_set_debug_led(1, true);

    enum handler_return ret = m68k_platform_irq(code);

    target_set_debug_led(1, false);

    if (ret == INT_RESCHEDULE) {
        thread_preempt();
    }
}
