/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
 * Copyright (c) 2015 Christopher Anderson
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
#include <debug.h>
#include <reg.h>
#include <stdio.h>
#include <string.h>
#include <dev/gpio.h>
#include <platform/gpio.h>
#include <platform/interrupts.h>
#include <target/gpioconfig.h>

#define MAX_GPIO 128

struct {
    int_handler callback;
    void *args;
} irq_callbacks[MAX_GPIO];

static enum handler_return gpio_int_handler(void *arg) {

    /* The mask register uses 1 to respresent masked, 0 for unmasked. Comparing that
     * register with the interrupt status register is the only way to determine
     * which gpio triggered the interrupt in the gic. */
    for (uint32_t bank = 0; bank < 4; bank++) {
        uint32_t mask = *REG32(GPIO_INT_MASK(bank));
        uint32_t stat = *REG32(GPIO_INT_STAT(bank));
        uint32_t active = ~mask & stat;

        if (active == 0) {
            continue;
        }

        //printf("mask 0x%08x stat 0x%08x active 0x%08x\n", mask, stat, active);
        while (active) {
            /* Find the rightmost set bit, calculate the associated gpio, and call the callback */
            uint16_t bit = 32 - clz(active) - 1;
            uint16_t gpio = bit + (bank * 32);

            active ^= (1 << bit);
            if (irq_callbacks[gpio].callback) {
                irq_callbacks[gpio].callback(irq_callbacks[gpio].args);
            }
            //printf("bit %u bank %u gpio %u was triggered\n", bit, bank, gpio);
        }

        *REG32(GPIO_INT_STAT(bank)) = stat;
    }

    return 0;
}

void zynq_unmask_gpio_interrupt(unsigned gpio)
{
    uint16_t bank = gpio / 31;
    uint16_t bit = gpio % 32;

    RMWREG32(GPIO_INT_EN(bank), bit, 1, 1);
    RMWREG32(GPIO_INT_STAT(bank), bit, 1, 1);
}

void zynq_mask_gpio_interrupt(unsigned gpio)
{
    uint16_t bank = gpio / 31;
    uint16_t bit = gpio % 32;

    RMWREG32(GPIO_INT_DIS(bank), bit, 1, 1);
}


void zynq_gpio_init(void)
{
    register_int_handler(GPIO_INT, gpio_int_handler, NULL);
    unmask_interrupt(GPIO_INT);
}

void zynq_gpio_early_init(void)
{
}

void register_gpio_int_handler(unsigned gpio, int_handler handler, void *args)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);
    DEBUG_ASSERT(handler);

    irq_callbacks[gpio].callback = handler;
    irq_callbacks[gpio].args = args;
}

void unregister_gpio_int_handler(unsigned gpio)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    irq_callbacks[gpio].callback = NULL;
    irq_callbacks[gpio].args = NULL;
}

int gpio_config(unsigned gpio, unsigned flags)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    uint16_t bank = gpio / 31;
    uint16_t bit = gpio % 32;
    uint32_t mio_cfg = MIO_GPIO;

    /* MIO region, exclude EMIO. MIO needs to be configured before the GPIO block. */
    if (bank < 2) {
        if (flags & GPIO_PULLUP) {
            mio_cfg |= MIO_PULLUP;
        }

        SLCR_REG(MIO_PIN_00 + (gpio * 4)) = mio_cfg;
    }

    if (flags & GPIO_OUTPUT || flags & GPIO_INPUT) {
        if (flags & GPIO_OUTPUT && flags & GPIO_INPUT) {
            printf("Cannot configure a gpio as both an input and output direction.\n");
            return -1;
        }

        RMWREG32(GPIO_DIRM(bank), bit, 1, ((flags & GPIO_OUTPUT) > 0));
        RMWREG32(GPIO_OEN(bank), bit, 1, ((flags & GPIO_OUTPUT) > 0));
    }

    if (flags & GPIO_EDGE || flags & GPIO_LEVEL) {
        if (flags & GPIO_EDGE && flags & GPIO_LEVEL) {
            printf("Cannot configure a gpio as both edge and level sensitive.\n");
            return -1;
        }
        RMWREG32(GPIO_INT_TYPE(bank), bit, 1, ((flags & GPIO_EDGE) > 0));
    }

    if (flags & GPIO_RISING || flags & GPIO_FALLING) {
        /* Zynq has a specific INT_ANY register for handling interrupts that trigger on both
         * rising and falling edges, but it specifically must only be used in edge mode */
        if (flags & GPIO_RISING && flags & GPIO_FALLING) {
            if ((flags & GPIO_EDGE) == 0) {
                printf("polarity must be rising or falling if level sensitivity is used.\n");
                return -1;
            }

            RMWREG32(GPIO_INT_ANY(bank), bit, 1, 1);
        } else {
            RMWREG32(GPIO_INT_POLARITY(bank), bit, 1, ((flags & GPIO_RISING) > 0));
            RMWREG32(GPIO_INT_ANY(bank), bit, 1, 0);
        }
    }


	return 0;
}

void gpio_set(unsigned gpio, unsigned on)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    uint16_t bank = gpio / 32;
    uint16_t bit = gpio % 32;
    uintptr_t reg = (bit < 16) ? GPIO_MASK_DATA_LSW(bank) : GPIO_MASK_DATA_MSW(bank);
    *REG32(reg) = (~(1 << bit) << 16) | (!!on << bit);
}

int gpio_get(unsigned gpio)
{
    DEBUG_ASSERT(gpio < MAX_GPIO);

    uint16_t bank = gpio / 32;
    uint16_t bit = gpio % 32;

    return ((*REG32(GPIO_DATA_RO(bank)) & (1 << bit)) > 0);
}

#include <lib/console.h>
#ifdef WITH_LIB_CONSOLE
static int cmd_zynq_gpio(int argc, const cmd_args *argv)
{
    for (unsigned int bank = 0; bank < 4; bank++) {
        printf("DIRM_%u (0x%08x):           0x%08x\n", bank, GPIO_DIRM(bank), *REG32(GPIO_DIRM(bank)));
        printf("OEN_%u (0x%08x):            0x%08x\n", bank, GPIO_OEN(bank), *REG32(GPIO_OEN(bank)));
        printf("INT_MASK_%u (0x%08x):       0x%08x\n", bank, GPIO_INT_MASK(bank), *REG32(GPIO_INT_MASK(bank)));
        printf("INT_STAT_%u (0x%08x):       0x%08x\n", bank, GPIO_INT_STAT(bank), *REG32(GPIO_INT_STAT(bank)));
        printf("INT_TYPE_%u (0x%08x):       0x%08x\n", bank, GPIO_INT_TYPE(bank), *REG32(GPIO_INT_TYPE(bank)));
        printf("INT_POLARITY_%u (0x%08x):   0x%08x\n", bank, GPIO_INT_POLARITY(bank), *REG32(GPIO_INT_POLARITY(bank)));
        printf("INT_ANY_%u (0x%08x):        0x%08x\n", bank, GPIO_INT_ANY(bank), *REG32(GPIO_INT_ANY(bank)));
    }
    return 0;
}
STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 1
{ "zynq_gpio", "Dump Zynq GPIO registers", &cmd_zynq_gpio },
#endif
STATIC_COMMAND_END(zynq_gpio);

#endif
