/* vim: set expandtab ts=4 sw=4 tw=100: */
/*
 * Copyright (c) 2013 Google Inc.
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

#include <assert.h>
#include <compiler.h>
#include <debug.h>
#include <printf.h>
#include <err.h>

#include <dev/gpio.h>
#include <dev/gpio_i2c.h>
#include <kernel/mutex.h>

#if (!(defined(GPIO_I2C_BUS_COUNT)) || (GPIO_I2C_BUS_COUNT <= 0))
#error ERROR: Must define GPIO_I2C_BUS_COUNT
#endif

typedef struct gpio_i2c_state {
    mutex_t                 lock;
    const gpio_i2c_info_t*  info;
} gpio_i2c_state_t;

static gpio_i2c_state_t gpio_i2c_states[GPIO_I2C_BUS_COUNT];

/******************************************************************************
 *
 * Internal implementation.
 *
 ******************************************************************************/
static inline void send_start(const gpio_i2c_info_t* i)
{
    gpio_config(i->sda, GPIO_OUTPUT);
    spin_cycles(i->qcd);
    gpio_config(i->scl, GPIO_OUTPUT);
    spin_cycles(i->hcd);
}

static inline void send_stop(const gpio_i2c_info_t* i)
{
    gpio_config(i->sda, GPIO_OUTPUT);
    gpio_config(i->scl, GPIO_INPUT);
    spin_cycles(i->qcd);
    gpio_config(i->sda, GPIO_INPUT);
}

static inline void send_restart(const gpio_i2c_info_t* i)
{
    gpio_config(i->scl, GPIO_INPUT);
    spin_cycles(i->qcd);
    send_start(i);
}

static inline void send_nack(const gpio_i2c_info_t* i)
{
    spin_cycles(i->hcd);
    gpio_config(i->scl, GPIO_INPUT);
    spin_cycles(i->hcd);
    gpio_config(i->scl, GPIO_OUTPUT);
    gpio_config(i->sda, GPIO_INPUT);
}

static inline void send_ack(const gpio_i2c_info_t* i)
{
    gpio_config(i->sda, GPIO_OUTPUT);
    send_nack(i);
}

static inline bool send_byte(const gpio_i2c_info_t* i, uint32_t b)
{
    bool ret;

    for (size_t j = 0; j < 8; ++j) {
        if (b & 0x80)
            gpio_config(i->sda, GPIO_INPUT);
        else
            gpio_config(i->sda, GPIO_OUTPUT);
        b <<= 1;
        /* setup time for data (the time between when data becomes stable and
         * clock becomes a stable high) is spec'ed to be 250ns for 100KHz i2c
         * and 100nsec for 400KHz i2c.  If any micro running LK needs to spin
         * here in order to hit that timing, they are welcome to add a spin
         * right here.
         */
        spin_cycles(i->hcd);
        gpio_config(i->scl, GPIO_INPUT);
        spin_cycles(i->hcd);
        gpio_config(i->scl, GPIO_OUTPUT);
    }

    gpio_config(i->sda, GPIO_INPUT);
    spin_cycles(i->hcd);
    gpio_config(i->scl, GPIO_INPUT);
    spin_cycles(i->hcd);
    ret = (0 == gpio_get(i->sda));
    gpio_config(i->scl, GPIO_OUTPUT);
    spin_cycles(i->hcd);

    return ret;
}

static inline void recv_byte(const gpio_i2c_info_t* i, uint8_t* b)
{
    uint32_t tmp = 0;

    for (size_t j = 0; j < 7; ++j) {
        gpio_config(i->scl, GPIO_INPUT);
        spin_cycles(i->hcd);
        if (gpio_get(i->sda))
            tmp |= 1;
        tmp <<= 1;
        gpio_config(i->scl, GPIO_OUTPUT);
        spin_cycles(i->hcd);
    }

    gpio_config(i->scl, GPIO_INPUT);
    spin_cycles(i->hcd);
    if (gpio_get(i->sda))
        tmp |= 1;
    gpio_config(i->scl, GPIO_OUTPUT);

    *b = (uint8_t)tmp;
}

static status_t gpio_i2c_tx_common(gpio_i2c_state_t* s,
                                   uint8_t address,
                                   const uint8_t* reg,
                                   const void* buf,
                                   size_t cnt)
{
    const gpio_i2c_info_t* i = s->info;
    status_t ret = ERR_I2C_NACK;

    DEBUG_ASSERT(buf || !cnt);

    mutex_acquire(&s->lock);
    send_start(i);
    if (!send_byte(i, address << 1))
        goto finished;

    if ((NULL != reg) && !send_byte(i, *reg))
        goto finished;

    for (size_t j = 0; j < cnt; ++j)
        if (!send_byte(i, ((const uint8_t*)buf)[j]))
            goto finished;

    ret = NO_ERROR;

finished:
    send_stop(i);
    mutex_release(&s->lock);
    return ret;
}

static status_t gpio_i2c_rx_common(gpio_i2c_state_t* s,
                                   uint8_t address,
                                   const uint8_t* reg,
                                   void* buf,
                                   size_t cnt)
{
    const gpio_i2c_info_t* i = s->info;
    status_t ret = ERR_I2C_NACK;

    DEBUG_ASSERT(buf && cnt);

    address <<= 1;

    mutex_acquire(&s->lock);
    send_start(i);
    if (!send_byte(i, address | (!reg ? 0x1 : 0x0)))
        goto finished;

    if (NULL != reg) {
        if (!send_byte(i, *reg))
            goto finished;

        send_restart(i);

        if (!send_byte(i, address | 0x1))
            goto finished;
    }

    recv_byte(i, buf++);
    for (size_t j = 0; j < (cnt - 1); ++j) {
        send_ack(i);
        recv_byte(i, buf++);
    }
    send_nack(i);
    ret = NO_ERROR;

finished:
    send_stop(i);
    mutex_release(&s->lock);
    return ret;
}

void gpio_i2c_add_bus(uint32_t bus_id, const gpio_i2c_info_t* info)
{
    gpio_i2c_state_t* s = gpio_i2c_states + bus_id;

    DEBUG_ASSERT(info);
    DEBUG_ASSERT(bus_id < GPIO_I2C_BUS_COUNT);
    DEBUG_ASSERT(!s->info);

    gpio_config(info->scl, GPIO_INPUT);
    gpio_config(info->sda, GPIO_INPUT);
    gpio_set(info->scl, 0);
    gpio_set(info->sda, 0);

    mutex_init(&s->lock);
    s->info = info;
}

/******************************************************************************
*
*  LK facing API
*
* *****************************************************************************/
void gpio_i2c_init_early(void) { }
void gpio_i2c_init(void) { }

status_t gpio_i2c_transmit(int bus, uint8_t address, const void* buf, size_t cnt)
{
    gpio_i2c_state_t* s = gpio_i2c_states + bus;
    if (((unsigned)bus >= countof(gpio_i2c_states)) || !s->info)
        return ERR_NOT_FOUND;

    return gpio_i2c_tx_common(s, address, NULL, buf, cnt);
}

status_t gpio_i2c_receive(int bus, uint8_t address, void* buf, size_t cnt)
{
    gpio_i2c_state_t* s = gpio_i2c_states + bus;
    if (((unsigned)bus >= countof(gpio_i2c_states)) || !s->info)
        return ERR_NOT_FOUND;

    return gpio_i2c_rx_common(s, address, NULL, buf, cnt);
}

status_t gpio_i2c_write_reg_bytes(int bus, uint8_t address, uint8_t reg, const uint8_t* buf, size_t cnt)
{
    gpio_i2c_state_t* s = gpio_i2c_states + bus;
    if (((unsigned)bus >= countof(gpio_i2c_states)) || !s->info)
        return ERR_NOT_FOUND;

    return gpio_i2c_tx_common(s, address, &reg, buf, cnt);
}

status_t gpio_i2c_read_reg_bytes(int bus, uint8_t address, uint8_t reg, uint8_t* buf, size_t cnt)
{
    gpio_i2c_state_t* s = gpio_i2c_states + bus;
    if (((unsigned)bus >= countof(gpio_i2c_states)) || !s->info) {
        return ERR_NOT_FOUND;
    }

    return gpio_i2c_rx_common(s, address, &reg, buf, cnt);
}

void i2c_init_early(void) __WEAK_ALIAS("gpio_i2c_init_early");
void i2c_init(void) __WEAK_ALIAS("gpio_i2c_init");
status_t i2c_transmit(int, uint8_t, const void*, size_t) __WEAK_ALIAS("gpio_i2c_transmit");
status_t i2c_receive(int, uint8_t, void*, size_t) __WEAK_ALIAS("gpio_i2c_receive");
status_t i2c_write_reg_bytes(int, uint8_t, uint8_t,
                             const uint8_t*, size_t) __WEAK_ALIAS("gpio_i2c_write_reg_bytes");
status_t i2c_read_reg_bytes(int, uint8_t, uint8_t,
                            uint8_t*, size_t) __WEAK_ALIAS("gpio_i2c_read_reg_bytes");
