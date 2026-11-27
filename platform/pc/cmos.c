/*
 * Copyright (c) 2019 Travis Geiselbrecht
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
#include <platform/cmos.h>

#include <kernel/spinlock.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <platform/pc.h>

#define CMOS_NMI_DISABLE 0x80

static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

uint8_t cmos_read(uint8_t reg) {
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    // NMI is masked (bit 7 of the index) for the access, then the index is parked on the
    // read only status register D with NMI enabled again
    outp(CMOS_CONTROL_REG, reg | CMOS_NMI_DISABLE);
    uint8_t val = inp(CMOS_DATA_REG);
    outp(CMOS_CONTROL_REG, CMOS_REG_STATUS_D);

    spin_unlock_irqrestore(&lock, state);

    return val;
}

uint16_t cmos_read_word(uint8_t reg) {
    return (uint16_t)(cmos_read(reg) | (cmos_read(reg + 1) << 8));
}
