/*
 * Copyright (c) 2009 Travis Geiselbrecht
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
#include <err.h>
#include <debug.h>
#include <reg.h>
#include <kernel/thread.h>
#include <dev/gpio.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/omap5912.h>
#include "platform_p.h"

#define LOCAL_TRACE 0

struct gpio_handlers {
	unsigned int flags;
	int_handler handler;
	void *arg;
};

static struct gpio_handlers handlers[NUM_GPIOS];

static inline addr_t gpio_base(unsigned int bank)
{
	DEBUG_ASSERT(bank >= 0 && bank < 4);

	switch (bank) {
		default:
		case 0: return GPIO0_BASE; break;
		case 1: return GPIO1_BASE; break;
		case 2: return GPIO2_BASE; break;
		case 3: return GPIO3_BASE; break;
	}
}

#define GPIO_TO_BANK(gpio) ((gpio) / 16)
#define GPIO_WITHIN_BANK(gpio) ((gpio) % 16)
#define GPIO_REG_BANK(bank, reg) (*REG32(gpio_base(bank) + (reg)))
#define GPIO_REG(gpio, reg) (*REG32(gpio_base(GPIO_TO_BANK(gpio)) + (reg)))

static enum handler_return gpio_int_handler(void *arg)
{
	enum handler_return res = INT_NO_RESCHEDULE;
	int bank = (int)arg;

	uint32_t status = GPIO_REG_BANK(bank, GPIO_IRQSTATUS1);
	LTRACEF("bank %d, status 0x%x\n", bank, status);

	while (status != 0) {
		int vector = __builtin_ctz(status);
		
		/* ack the gpio */
		GPIO_REG_BANK(bank, GPIO_IRQSTATUS1) |= (1 << vector);
		status &= ~(1 << vector);

		/* call the handler */
		vector += bank * 16;
		if (handlers[vector].handler) {
			res |= handlers[vector].handler(handlers[vector].arg);
		}
	}

	return res;
}

void platform_init_gpio(void)
{
	GPIO_REG_BANK(0, GPIO_CLEAR_IRQENABLE1) = 0xffffffff;
	GPIO_REG_BANK(0, GPIO_CLEAR_IRQENABLE2) = 0xffffffff;
	GPIO_REG_BANK(1, GPIO_CLEAR_IRQENABLE1) = 0xffffffff;
	GPIO_REG_BANK(1, GPIO_CLEAR_IRQENABLE2) = 0xffffffff;
	GPIO_REG_BANK(2, GPIO_CLEAR_IRQENABLE1) = 0xffffffff;
	GPIO_REG_BANK(2, GPIO_CLEAR_IRQENABLE2) = 0xffffffff;
	GPIO_REG_BANK(3, GPIO_CLEAR_IRQENABLE1) = 0xffffffff;
	GPIO_REG_BANK(3, GPIO_CLEAR_IRQENABLE2) = 0xffffffff;

	register_int_handler(IRQ_GPIO1, &gpio_int_handler, (void *)0);
	register_int_handler(IRQ_GPIO2, &gpio_int_handler, (void *)1);
	register_int_handler(IRQ_GPIO3, &gpio_int_handler, (void *)2);
	register_int_handler(IRQ_GPIO4, &gpio_int_handler, (void *)3);

	unmask_interrupt(IRQ_GPIO1);
	unmask_interrupt(IRQ_GPIO2);
	unmask_interrupt(IRQ_GPIO3);
	unmask_interrupt(IRQ_GPIO4);
}

int gpio_config(unsigned nr, unsigned flags)
{
	/* set direction based on flags */
	if (flags & GPIO_OUTPUT) {
		GPIO_REG(nr, GPIO_DIRECTION) &= ~(1 << GPIO_WITHIN_BANK(nr));
	} else {
		GPIO_REG(nr, GPIO_DIRECTION) |= (1 << GPIO_WITHIN_BANK(nr));
	}

	return 0;
}

void gpio_set(unsigned nr, unsigned on)
{
	if (on)
		GPIO_REG(nr, GPIO_SET_DATAOUT) = (1 << GPIO_WITHIN_BANK(nr));
	else
		GPIO_REG(nr, GPIO_CLEAR_DATAOUT) = (1 << GPIO_WITHIN_BANK(nr));
}

int gpio_get(unsigned nr)
{
	TRACEF("nr %d dat 0x%x irqstat 0x%x\n", nr, GPIO_REG(nr, GPIO_DATAIN), GPIO_REG(nr, GPIO_IRQSTATUS1));
	return GPIO_REG(nr, GPIO_DATAIN) & (1 << GPIO_WITHIN_BANK(nr));
}

int gpio_set_interrupt(unsigned int nr, unsigned flags, int_handler handler, void *arg)
{
	uint32_t bits = 0;

	if (flags & GPIO_EDGE) {
		if (flags & GPIO_RISING)
			bits |= (1 << ((nr % 8) * 2 + 1));
		if (flags & GPIO_FALLING)
			bits |= (1 << ((nr % 8) * 2));
	} else { // GPIO_LEVEL
		panic("level triggered interrupts are not supported\n");
	}

	enter_critical_section();

	gpio_config(nr, GPIO_INPUT);

	if (GPIO_WITHIN_BANK(nr) < 8)
		GPIO_REG(nr, GPIO_EDGE_CTRL1) = (GPIO_REG(nr, GPIO_EDGE_CTRL1) & ~(3 << ((nr % 8) * 2))) | bits;
	else 
		GPIO_REG(nr, GPIO_EDGE_CTRL2) = (GPIO_REG(nr, GPIO_EDGE_CTRL2) & ~(3 << ((nr % 8) * 2))) | bits;

	GPIO_REG(nr, GPIO_IRQSTATUS1) = (1 << GPIO_WITHIN_BANK(nr));
	GPIO_REG(nr, GPIO_SET_IRQENABLE1) = (1 << GPIO_WITHIN_BANK(nr));

	handlers[nr].flags = flags;
	handlers[nr].handler = handler;
	handlers[nr].arg = arg;

	exit_critical_section();

	return 0;
}

