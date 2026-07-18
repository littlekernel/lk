//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <stdint.h>

#include <arch/interrupts.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lib/cbuf.h>
#include <platform/debug.h>
#include <platform/interrupts.h>

#include "platform_p.h"

namespace {

enum : uint8_t {
    CHANNEL_B_CONTROL_REG = 0,
    CHANNEL_B_DATA_REG = 2,
    CHANNEL_A_CONTROL_REG = 4,
    CHANNEL_A_DATA_REG = 6,

    WR1 = 1,
    WR2 = 2,
    WR9 = 9,
    WR15 = 15,

    RR0 = 0,
    RR1 = 1,
    RR2 = 2,
    RR3 = 3,

    WR0_RESET_EXT_STATUS_INT = 0x10,
    WR0_RESET_TX_INT_PENDING = 0x28,
    WR0_ERROR_RESET = 0x30,
    WR0_RESET_HIGHEST_IUS = 0x38,

    WR1_TX_INT_ENABLE = 0x02,
    WR1_INT_ALL_RX = 0x10,

    WR9_MASTER_INT_ENABLE = 0x08,
    WR9_SOFTWARE_INTACK_ENABLE = 0x20,

    RR0_RX_CHAR_AVAILABLE = 0x01,
    RR0_TX_BUFFER_EMPTY = 0x04,

    RR1_PARITY_ERROR = 0x10,
    RR1_RX_OVERRUN = 0x20,
    RR1_FRAMING_ERROR = 0x40,
    RR1_ERROR_MASK = RR1_PARITY_ERROR | RR1_RX_OVERRUN | RR1_FRAMING_ERROR,

    RR3_CHANNEL_A_EXT_STATUS_IP = 0x08,
    RR3_CHANNEL_A_TX_IP = 0x10,
    RR3_CHANNEL_A_RX_IP = 0x20,
};

// Real physical 36-bit addresses.
constexpr uint64_t SCC_BASE = 0xf'f110'0000ULL;

constexpr size_t RXBUF_SIZE = 128;
constexpr size_t TXBUF_SIZE = 256;

char rx_buf_data[RXBUF_SIZE];
char tx_buf_data[TXBUF_SIZE];
cbuf_t rx_buf;
cbuf_t tx_buf;
SpinLock tx_lock{};

uint8_t channel_a_wr1 = 0;
bool tx_irq_enabled = false;
bool scc_initialized = false;

inline uint8_t scc_read_control(uint8_t reg_offset) {
    return sparc_read_physical_8(SCC_BASE + reg_offset);
}

inline void scc_write_control(uint8_t reg_offset, uint8_t value) {
    sparc_write_physical_8(SCC_BASE + reg_offset, value);
}

inline uint8_t scc_read_data(uint8_t reg_offset) {
    return sparc_read_physical_8(SCC_BASE + reg_offset);
}

inline void scc_write_data(uint8_t reg_offset, uint8_t value) {
    sparc_write_physical_8(SCC_BASE + reg_offset, value);
}

uint8_t scc_read_reg(uint8_t control_reg, uint8_t reg) {
    if (reg != RR0) {
        scc_write_control(control_reg, reg);
    }
    return scc_read_control(control_reg);
}

void scc_write_reg(uint8_t control_reg, uint8_t reg, uint8_t value) {
    scc_write_control(control_reg, reg);
    scc_write_control(control_reg, value);
}

void scc_write_command(uint8_t control_reg, uint8_t command) {
    scc_write_control(control_reg, command);
}

void scc_write_wr1_locked(uint8_t value) {
    channel_a_wr1 = value;
    scc_write_reg(CHANNEL_A_CONTROL_REG, WR1, channel_a_wr1);
}

void scc_disable_tx_interrupt_locked(void) {
    if (!tx_irq_enabled) {
        return;
    }

    tx_irq_enabled = false;
    scc_write_wr1_locked(channel_a_wr1 & static_cast<uint8_t>(~WR1_TX_INT_ENABLE));
    scc_write_command(CHANNEL_A_CONTROL_REG, WR0_RESET_TX_INT_PENDING);
}

void scc_enable_tx_interrupt_locked(void) {
    if (tx_irq_enabled) {
        return;
    }

    tx_irq_enabled = true;
    scc_write_wr1_locked(channel_a_wr1 | WR1_TX_INT_ENABLE);
}

bool scc_tx_service_locked(void) {
    if ((scc_read_reg(CHANNEL_A_CONTROL_REG, RR0) & RR0_TX_BUFFER_EMPTY) == 0) {
        return false;
    }

    char c;
    if (cbuf_read_char(&tx_buf, &c, false) != 1) {
        scc_disable_tx_interrupt_locked();
        return false;
    }

    scc_write_data(CHANNEL_A_DATA_REG, static_cast<uint8_t>(c));
    return true;
}

void scc_kick_tx_locked(void) {
    scc_enable_tx_interrupt_locked();
    scc_tx_service_locked();
}

void scc_flush_tx_polled_locked(void) {
    char queued;
    while (cbuf_read_char(&tx_buf, &queued, false) == 1) {
        while ((scc_read_reg(CHANNEL_A_CONTROL_REG, RR0) & RR0_TX_BUFFER_EMPTY) == 0) {
        }
        scc_write_data(CHANNEL_A_DATA_REG, static_cast<uint8_t>(queued));
    }
}

handler_return scc_rx_irq(void) {
    handler_return ret = INT_NO_RESCHEDULE;

    while ((scc_read_reg(CHANNEL_A_CONTROL_REG, RR0) & RR0_RX_CHAR_AVAILABLE) != 0) {
        char c = static_cast<char>(scc_read_data(CHANNEL_A_DATA_REG));
        cbuf_write_char(&rx_buf, c, false);
        ret = INT_RESCHEDULE;

        if ((scc_read_reg(CHANNEL_A_CONTROL_REG, RR1) & RR1_ERROR_MASK) != 0) {
            scc_write_command(CHANNEL_A_CONTROL_REG, WR0_ERROR_RESET);
        }
    }

    return ret;
}

void scc_status_irq(void) {
    (void)scc_read_reg(CHANNEL_A_CONTROL_REG, RR0);
    scc_write_command(CHANNEL_A_CONTROL_REG, WR0_RESET_EXT_STATUS_INT);
}

handler_return sun4m_scc_irq(void *) {
    handler_return ret = INT_NO_RESCHEDULE;

    for (;;) {
        uint8_t pending = scc_read_reg(CHANNEL_A_CONTROL_REG, RR3);
        uint8_t channel_a_pending =
            pending & static_cast<uint8_t>(RR3_CHANNEL_A_RX_IP | RR3_CHANNEL_A_TX_IP |
                                           RR3_CHANNEL_A_EXT_STATUS_IP);
        if (channel_a_pending == 0) {
            break;
        }

        (void)scc_read_reg(CHANNEL_B_CONTROL_REG, RR2);

        if ((channel_a_pending & RR3_CHANNEL_A_RX_IP) != 0) {
            if (scc_rx_irq() == INT_RESCHEDULE) {
                ret = INT_RESCHEDULE;
            }
        } else if ((channel_a_pending & RR3_CHANNEL_A_TX_IP) != 0) {
            AutoSpinLockNoIrqSave guard(&tx_lock);
            scc_tx_service_locked();
        } else {
            scc_status_irq();
        }

        scc_write_command(CHANNEL_A_CONTROL_REG, WR0_RESET_HIGHEST_IUS);
    }

    return ret;
}

} // namespace

void sun4m_scc_early_init(void) {
    tx_irq_enabled = false;
    scc_initialized = false;
}

void sun4m_scc_init(void) {
    cbuf_initialize_etc(&rx_buf, RXBUF_SIZE, rx_buf_data);
    cbuf_initialize_etc(&tx_buf, TXBUF_SIZE, tx_buf_data);

    {
        AutoSpinLock guard(&tx_lock);

        channel_a_wr1 = 0;
        tx_irq_enabled = false;

        while ((scc_read_reg(CHANNEL_A_CONTROL_REG, RR0) & RR0_RX_CHAR_AVAILABLE) != 0) {
            (void)scc_read_data(CHANNEL_A_DATA_REG);
        }

        scc_write_command(CHANNEL_A_CONTROL_REG, WR0_ERROR_RESET);
        scc_write_command(CHANNEL_A_CONTROL_REG, WR0_RESET_EXT_STATUS_INT);
        scc_write_command(CHANNEL_A_CONTROL_REG, WR0_RESET_TX_INT_PENDING);
        scc_write_command(CHANNEL_A_CONTROL_REG, WR0_RESET_HIGHEST_IUS);

        scc_write_reg(CHANNEL_B_CONTROL_REG, WR2, 0);
        scc_write_reg(CHANNEL_A_CONTROL_REG, WR15, 0);
        scc_write_reg(CHANNEL_A_CONTROL_REG, WR9,
                      WR9_MASTER_INT_ENABLE | WR9_SOFTWARE_INTACK_ENABLE);
        scc_write_wr1_locked(WR1_INT_ALL_RX);
    }

    register_int_handler(SUN4M_SCC_IRQ, sun4m_scc_irq, nullptr);
    unmask_interrupt(SUN4M_SCC_IRQ);

    scc_initialized = true;
}

void platform_pputc(char c) {
    if (c == '\n') {
        platform_pputc('\r');
    }

    AutoSpinLock guard(&tx_lock);
    if (scc_initialized) {
        scc_flush_tx_polled_locked();
    }
    while ((scc_read_reg(CHANNEL_A_CONTROL_REG, RR0) & RR0_TX_BUFFER_EMPTY) == 0) {
    }
    scc_write_data(CHANNEL_A_DATA_REG, static_cast<uint8_t>(c));
}

int platform_pgetc(char *c, bool wait) {
    do {
        if ((scc_read_reg(CHANNEL_A_CONTROL_REG, RR0) & RR0_RX_CHAR_AVAILABLE) != 0) {
            *c = static_cast<char>(scc_read_data(CHANNEL_A_DATA_REG));
            return 0;
        }
    } while (wait);

    return -1;
}

void platform_dputc(char c) {
    if (c == '\n') {
        platform_dputc('\r');
    }

    if (!scc_initialized || arch_ints_disabled()) {
        platform_pputc(c);
        return;
    }

    while (cbuf_write_char(&tx_buf, c, false) != 1) {
        thread_yield();
    }

    AutoSpinLock guard(&tx_lock);
    scc_kick_tx_locked();
}

int platform_dgetc(char *c, bool wait) {
    if (!scc_initialized) {
        return platform_pgetc(c, wait);
    }

    if (cbuf_read_char(&rx_buf, c, wait) == 1) {
        return 0;
    }

    return -1;
}
