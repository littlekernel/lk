/*
 * Copyright (c) 2017 The Fuchsia Authors
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
#include <arch/arm/cm.h>
#include <dev/i2c.h>
#include <err.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <platform/rcc.h>
#include <stm32f0xx.h>

typedef I2C_TypeDef stm32_i2c_regs_t;

// Some supplemental defines.
#define I2C_CR2_NBYTES_SHIFT 16


typedef enum {
    STM32_I2C_STATE_IDLE = 0,
    STM32_I2C_STATE_REG,
    STM32_I2C_STATE_TX,
    STM32_I2C_STATE_RX,

    // Error states have bit 0x100 set.
    STM32_I2C_STATE_ERROR                  = 0x100,
    STM32_I2C_STATE_ERROR_TIMEOUT          = 0x101,
    STM32_I2C_STATE_ERROR_ARBITRATION_LOST = 0x102,
    STM32_I2C_STATE_ERROR_BUS_ERROR        = 0x103,
    STM32_I2C_STATE_ERROR_NACK             = 0x104,
} stm32_i2c_state_t;

typedef enum {
    STM32_I2C_DIR_R = I2C_CR2_RD_WRN,
    STM32_I2C_DIR_W = 0,
} stm32_i2c_dir_t;

typedef struct {
    uint8_t *data;
    uint16_t len;
    uint16_t pos;
} stm32_i2c_buf_t;

typedef struct {
    stm32_rcc_clk_t clock;
    stm32_i2c_regs_t *regs;

    stm32_i2c_state_t state;

    uint8_t reg;
    stm32_i2c_buf_t tx_buf;
    stm32_i2c_buf_t rx_buf;

    mutex_t lock;  // Used to ensure that only one transaction is active.
    event_t txn_complete;
} stm32_i2c_dev_t;

#ifdef ENABLE_I2C1
static stm32_i2c_dev_t i2c1 = {
    .clock = STM32_RCC_CLK_I2C1,
    .regs = I2C1,
};
#endif

#ifdef ENABLE_I2C2
static stm32_i2c_dev_t i2c2 = {
    .clock = STM32_RCC_CLK_I2C2,
    .regs = I2C2,
};
#endif

stm32_i2c_dev_t *stm32_i2c_get_dev(int bus) {

    switch (bus) {
#ifdef ENABLE_I2C1
        case 1:
            return &i2c1;
#endif
#ifdef ENABLE_I2C2
        case 2:
            return &i2c2;
#endif
        default:
            return NULL;
    }
}

static void stm32_i2c_early_init(stm32_i2c_dev_t *i2c) {
    stm32_rcc_set_enable(i2c->clock, true);

    // Clear PE.
    i2c->regs->CR1 &= ~I2C_CR1_PE;

    // Leave the default analog filter enabled.

    // Magic timing value for I2C from an 8Mhz clock.  Calculated by STM32CubeMX.
    // Standard Mode (100 KHz): 0x2000090e
    // Fast Mode (400 KHz):     0x0000020b
    // Fast Mode Plus:          0x00000001
    //i2c->regs->TIMINGR = 0x2000090E;
    i2c->regs->TIMINGR = 0x0000020b;

    // Configure NOSTRETCH in CR1.
    // Must be kept cleared in master mode.

    // Set PE
    i2c->regs->CR1 |= I2C_CR1_PE | I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_NACKIE | I2C_CR1_ERRIE
        | I2C_CR1_TCIE;

    i2c->state = STM32_I2C_STATE_IDLE;
    mutex_init(&i2c->lock);
    event_init(&i2c->txn_complete, false, 0);
}

void i2c_init(void) {

}

void i2c_init_early(void) {
#ifdef ENABLE_I2C1
    stm32_i2c_early_init(&i2c1);
    NVIC_EnableIRQ(I2C1_IRQn);
#endif
#ifdef ENABLE_I2C2
    stm32_i2c_early_init(&i2c2);
    NVIC_EnableIRQ(I2C2_IRQn);
#endif
}

static bool stm32_i2c_buf_is_done(stm32_i2c_buf_t *buf) {
    return buf->pos >= buf->len;
}

static uint8_t stm32_i2c_buf_pop(stm32_i2c_buf_t *buf) {
    return buf->data[buf->pos++];
}

static void stm32_i2c_buf_push(stm32_i2c_buf_t *buf, uint8_t data) {
   buf->data[buf->pos++] = data;
}

static bool stm32_i2c_check_error(stm32_i2c_dev_t *i2c,
                                  uint32_t isr,
                                  uint32_t flag,
                                  stm32_i2c_state_t error_state) {
    if (isr & flag) {
        i2c->state = error_state;
        return true;
    } else {
        return false;
    }
}

static bool stm32_i2c_is_error_state(stm32_i2c_state_t state) {
    return state & STM32_I2C_STATE_ERROR;
}

static bool stm32_i2c_is_terminal_state(stm32_i2c_state_t state) {
    return state == STM32_I2C_STATE_IDLE || stm32_i2c_is_error_state(state);
}

static void stm32_i2c_do_repeated_start(stm32_i2c_dev_t *i2c) {
    uint32_t cr2 = i2c->regs->CR2;
    cr2 &= ~(I2C_CR2_NBYTES | I2C_CR2_START);
    cr2 |= i2c->rx_buf.len << I2C_CR2_NBYTES_SHIFT | I2C_CR2_RD_WRN;
    i2c->regs->CR2 = cr2;
    i2c->regs->CR2 = cr2 | I2C_CR2_START;
}

static void stm32_i2c_do_stop(stm32_i2c_dev_t *i2c) {
    i2c->regs->CR2 |= I2C_CR2_STOP;
}

static void stm32_i2c_irq(stm32_i2c_dev_t *i2c) {
    arm_cm_irq_entry();
    uint32_t isr = i2c->regs->ISR;

    bool error = false;
    error |= stm32_i2c_check_error(i2c, isr, I2C_ISR_NACKF, STM32_I2C_STATE_ERROR_NACK);
    error |= stm32_i2c_check_error(i2c, isr, I2C_ISR_BERR, STM32_I2C_STATE_ERROR_BUS_ERROR);
    error |= stm32_i2c_check_error(i2c, isr, I2C_ISR_ARLO, STM32_I2C_STATE_ERROR_ARBITRATION_LOST);
    error |= stm32_i2c_check_error(i2c, isr, I2C_ISR_TIMEOUT, STM32_I2C_STATE_ERROR_TIMEOUT);
    if (error) {
        goto out;
    }

    switch (i2c->state) {
        case STM32_I2C_STATE_REG:
            if (isr & I2C_ISR_TXIS) {
                i2c->regs->TXDR = i2c->reg;
                i2c->state = STM32_I2C_STATE_TX;
            }
            break;

        case STM32_I2C_STATE_TX:
            if (isr & I2C_ISR_TXIS) {
                if (!stm32_i2c_buf_is_done(&i2c->tx_buf)) {
                    i2c->regs->TXDR = stm32_i2c_buf_pop(&i2c->tx_buf);
                }
            }
            if (isr & I2C_ISR_TC) {
                if (!stm32_i2c_buf_is_done(&i2c->rx_buf)) {
                    i2c->state = STM32_I2C_STATE_RX;
                    stm32_i2c_do_repeated_start(i2c);
                } else {
                    i2c->state = STM32_I2C_STATE_IDLE;
                    stm32_i2c_do_stop(i2c);
                }
            }
            break;

        case STM32_I2C_STATE_RX:
            if (isr & I2C_ISR_RXNE) {
                stm32_i2c_buf_push(&i2c->rx_buf, i2c->regs->RXDR);
            }
            if (isr & I2C_ISR_TC) {
                if (stm32_i2c_buf_is_done(&i2c->rx_buf)) {
                    i2c->state = STM32_I2C_STATE_IDLE;
                    stm32_i2c_do_stop(i2c);
                }
            }
            break;

        default:
            break;
    }

out:
    i2c->regs->ICR = isr;
    if (stm32_i2c_is_terminal_state(i2c->state)) {
        event_signal(&i2c->txn_complete, false);
        arm_cm_irq_exit(true);
    } else {
        arm_cm_irq_exit(false);
    }
}

#ifdef ENABLE_I2C1
void stm32_I2C1_IRQ(void)
{
    stm32_i2c_irq(&i2c1);
}
#endif

#ifdef ENABLE_I2C2
void stm32_I2C2_IRQ(void)
{
    stm32_i2c_irq(&i2c2);
}
#endif

static void stm32_i2c_setup_buf(stm32_i2c_buf_t *buf, uint8_t *data, size_t count) {
    buf->data = data;
    buf->len = count;
    buf->pos = 0;
}

// 10 bit addressing not supported.
// Transfers > 255 bytes not supported.
static status_t stm32_i2c_txn(stm32_i2c_dev_t *i2c, uint8_t address,
                                uint8_t *reg_addr,
                                uint8_t *tx_data, size_t tx_count,
                                uint8_t *rx_data, size_t rx_count) {
    mutex_acquire(&i2c->lock);
    stm32_i2c_setup_buf(&i2c->tx_buf, tx_data, tx_count);
    stm32_i2c_setup_buf(&i2c->rx_buf, rx_data, rx_count);

    if (reg_addr != NULL) {
        tx_count++;
        i2c->reg = *reg_addr;
    }

    stm32_i2c_dir_t dir;
    size_t count;
    if (tx_count > 0) {
        count = tx_count;
        dir = STM32_I2C_DIR_W;
        i2c->state = reg_addr ? STM32_I2C_STATE_REG : STM32_I2C_STATE_TX;
    } else {
        count = rx_count;
        dir = STM32_I2C_DIR_R;
        i2c->state = STM32_I2C_STATE_RX;
    }


    event_unsignal(&i2c->txn_complete);
    i2c->regs->CR2 = (address & 0x7f) << 1 | dir | (count & 0xff) << I2C_CR2_NBYTES_SHIFT
        | I2C_CR2_START;
    event_wait(&i2c->txn_complete);

    stm32_i2c_state_t state = i2c->state;

    mutex_release(&i2c->lock);

    switch (state) {
        case STM32_I2C_STATE_IDLE:
            return NO_ERROR;

        case STM32_I2C_STATE_ERROR_TIMEOUT:
            return ERR_TIMED_OUT;

        case STM32_I2C_STATE_ERROR_ARBITRATION_LOST:
            return ERR_BUSY;

        case STM32_I2C_STATE_ERROR_BUS_ERROR:
            return ERR_IO;

        case STM32_I2C_STATE_ERROR_NACK:
            return ERR_I2C_NACK;

        default:
            return ERR_BAD_STATE;
    }
}


status_t i2c_transmit(int bus, uint8_t address, const void *buf, size_t count) {
    stm32_i2c_dev_t *i2c = stm32_i2c_get_dev(bus);
    if (i2c == NULL) {
        return ERR_BAD_HANDLE;
    }

    // We discard const here in service of keeping the rx and tx buffer handling code common.
    return stm32_i2c_txn(i2c, address, NULL, (uint8_t *)buf, count, NULL, 0);
}

status_t i2c_receive(int bus, uint8_t address, void *buf, size_t count) {
    stm32_i2c_dev_t *i2c = stm32_i2c_get_dev(bus);
    if (i2c == NULL) {
        return ERR_BAD_HANDLE;
    }

    return stm32_i2c_txn(i2c, address, NULL, NULL, 0, buf, count);
}

status_t i2c_write_reg_bytes(int bus, uint8_t address, uint8_t reg, const uint8_t *buf, size_t count) {
    stm32_i2c_dev_t *i2c = stm32_i2c_get_dev(bus);
    if (i2c == NULL) {
        return ERR_BAD_HANDLE;
    }

    // We discard const here in service of keeping the rx and tx buffer handling code common.
    return stm32_i2c_txn(i2c, address, &reg, (uint8_t *)buf, count, NULL, 0);
}

status_t i2c_read_reg_bytes(int bus, uint8_t address, uint8_t reg, uint8_t *buf, size_t count) {
    stm32_i2c_dev_t *i2c = stm32_i2c_get_dev(bus);

    return stm32_i2c_txn(i2c, address, &reg, NULL, 0, buf, count);
}
