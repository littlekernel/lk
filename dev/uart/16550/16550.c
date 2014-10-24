/*
 * Copyright (c) 2012 Corey Tabaka
 * Copyright (c) 2014 Xiaomi Inc.
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
#include <malloc.h>
#include <reg.h>
#include <stdio.h>
#include <dev/class/uart.h>
#include <dev/uart/16550.h>
#include <platform/interrupts.h>

#define UART_RBR			0
#define UART_THR			0
#define UART_IER			1
#define UART_IIR			2
#define UART_FCR			2
#define UART_LCR			3
#define UART_MCR			4
#define UART_LSR			5
#define UART_MSR			6
#define UART_SCR			7
#define UART_DLL			0
#define UART_DLM			1

#define UART_IER_ERBFI			0x01
#define UART_IER_ETBEI			0x02
#define UART_IER_ELSI			0x04
#define UART_IER_EDSSI			0x08

#define UART_IIR_DSSI			0x00
#define UART_IIR_NONE			0x01
#define UART_IIR_TBEI			0x02
#define UART_IIR_RBFI			0x04
#define UART_IIR_LSI			0x06
#define UART_IIR_TIMEOUT		0x0C
#define UART_IIR_MASK			0x0F

#define UART_FCR_FIFO_ENABLE		0x01
#define UART_FCR_RCVR_RESET		0x02
#define UART_FCR_XMIT_RESET		0x04
#define UART_FCR_DMA_SELECT		0x08
#define UART_FCR_RCVR_TRIG1		0x00
#define UART_FCR_RCVR_TRIG4		0x40
#define UART_FCR_RCVR_TRIG8		0x80
#define UART_FCR_RCVR_TRIG14		0xC0

#define UART_LCR_WLEN5			0x00
#define UART_LCR_WLEN6			0x01
#define UART_LCR_WLEN7			0x02
#define UART_LCR_WLEN8			0x03
#define UART_LCR_STOP			0x04
#define UART_LCR_PARITY			0x08
#define UART_LCR_EVEN			0x10
#define UART_LCR_STICK			0x20
#define UART_LCR_BREAK			0x40
#define UART_LCR_DLAB			0x80

#define UART_MCR_DTR			0x01
#define UART_MCR_RTS			0x02
#define UART_MCR_OUT1			0x04
#define UART_MCR_OUT2			0x08
#define UART_MCR_LOOP			0x10
#define UART_MCR_AFE			0x20

#define UART_LSR_DR			0x01
#define UART_LSR_OE			0x02
#define UART_LSR_PE			0x04
#define UART_LSR_FE			0x08
#define UART_LSR_BI			0x10
#define UART_LSR_THRE			0x20
#define UART_LSR_TEMT			0x40
#define UART_LSR_FIFOE			0x80

// internal function
static uint8_t uart_get(struct device *dev, off_t offset)
{
	const struct uart_16550_config* config = dev->config;
	uintptr_t addr = config->base + offset * config->stride;

	switch (config->unit) {
	case 1:
#ifdef UART_16550_IOPORT
		return inp(addr);
#else
		return readb(addr);
#endif
	case 2:
#ifdef UART_16550_IOPORT
		return inpw(addr);
#else
		return readw(addr);
#endif
	case 4:
#ifdef UART_16550_IOPORT
		return inpd(addr);
#else
		return readl(addr);
#endif
	default:
		return 0;
	}
}

static void uart_set(struct device *dev, off_t offset, uint8_t value)
{
	const struct uart_16550_config* config = dev->config;
	uintptr_t addr = config->base + offset * config->stride;

	switch (config->unit) {
	case 1:
#ifdef UART_16550_IOPORT
		outp(addr, value);
#else
		writeb(value, addr);
#endif
		break;
	case 2:
#ifdef UART_16550_IOPORT
		outpw(addr, value);
#else
		writew(value, addr);
#endif
		break;
	case 4:
#ifdef UART_16550_IOPORT
		outpd(addr, value);
#else
		writel(value, addr);
#endif
		break;
	}
}

static void uart_set_baud(struct device *dev, unsigned baud)
{
	const struct uart_16550_config* config = dev->config;
	unsigned divisor = config->clock_rate / (16 * baud);
	uint8_t lcr = uart_get(dev, UART_LCR);

	uart_set(dev, UART_LCR, lcr | UART_LCR_DLAB);
	uart_set(dev, UART_DLL, divisor);
	uart_set(dev, UART_DLM, divisor >> 8);
	uart_set(dev, UART_LCR, lcr);
}

static void uart_init_baud(struct device *dev)
{
	const struct uart_16550_config* config = dev->config;
	uart_set_baud(dev, config->baud_rate);
}

static void uart_init_fcr(struct device *dev)
{
	uart_set(dev, UART_FCR, UART_FCR_RCVR_RESET | UART_FCR_XMIT_RESET);
	uart_set(dev, UART_FCR, UART_FCR_FIFO_ENABLE);
}

static void uart_init_lcr(struct device *dev)
{
	const struct uart_16550_config* config = dev->config;
	uint8_t lcr = 0;

	switch (config->word_length) {
	case 5:
		lcr |= UART_LCR_WLEN5;
		break;
	case 6:
		lcr |= UART_LCR_WLEN6;
		break;
	case 7:
		lcr |= UART_LCR_WLEN7;
		break;
	case 8:
		lcr |= UART_LCR_WLEN8;
		break;
	}

	lcr |= config->stop_bits > 1 ? UART_LCR_STOP : 0;
	lcr |= config->parity_enable ? UART_LCR_PARITY : 0;
	lcr |= config->even_parity ? UART_LCR_EVEN : 0;

	uart_set(dev, UART_LCR, lcr);
}

static void uart_init_mcr(struct device *dev)
{
	const struct uart_16550_config* config = dev->config;
	if (config->autoflow_enable) {
		uint8_t mcr = uart_get(dev, UART_MCR);
		mcr |= UART_MCR_AFE;
		uart_set(dev, UART_MCR, mcr);
	}
}

static void uart_init_config(struct device *dev)
{
	uart_init_baud(dev);
	uart_init_fcr(dev);
	uart_init_lcr(dev);
	uart_init_mcr(dev);
}

static enum handler_return uart_irq_handler(void *arg)
{
	bool resched = false;
	struct device *dev = arg;
	struct uart_16550_state *state = dev->state;

	while (uart_get(dev, UART_LSR) & UART_LSR_DR) {
		uint8_t c = uart_get(dev, UART_RBR);
		cbuf_write(&state->rxbuf, &c, 1, false);
		resched = true;
	}

	return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

static void uart_init_irq(struct device *dev)
{
	const struct uart_16550_config* config = dev->config;
	uart_set(dev, UART_IER, UART_IER_ERBFI | UART_IER_ELSI);
	register_int_handler(config->irq, uart_irq_handler, dev);
	unmask_interrupt(config->irq);
}

static void uart_fini_irq(struct device *dev)
{
	const struct uart_16550_config* config = dev->config;
	mask_interrupt(config->irq);
	uart_set(dev, UART_IER, 0);
}

// new uart api
static status_t uart_init(struct device *dev)
{
	if (!dev->state) {
		const struct uart_16550_config* config = dev->config;
		if (!dev->config) {
			return ERR_NOT_CONFIGURED;
		}

		struct uart_16550_state *state = config->state;
		if (!state) {
			state = malloc(sizeof(*state));
			if (!state) {
				return ERR_NO_MEMORY;
			}
		}

		cbuf_initialize_etc(&state->rxbuf, UART_RXBUF_SIZE, state->buf);
		dev->state = state;

		uart_init_config(dev);
		uart_init_irq(dev);
	}
	return NO_ERROR;
}

static status_t uart_fini(struct device *dev)
{
	const struct uart_16550_config* config = dev->config;
	if (dev->state) {
		uart_fini_irq(dev);
		if (dev->state != config->state) {
			free(dev->state);
		}
		dev->state = NULL;
	}
	return NO_ERROR;
}

static ssize_t uart_read(struct device *dev, void *buf, size_t len)
{
	ssize_t ret = uart_init(dev);
	if (ret >= 0) {
		struct uart_16550_state *state = dev->state;
		ret = cbuf_read(&state->rxbuf, buf, len, true);
	}
	return ret;
}

static ssize_t uart_write(struct device *dev, const void *buf_, size_t len)
{
	ssize_t ret = uart_init(dev);
	if (ret >= 0) {
		const char *buf = buf_;
		for (size_t i = 0; i < len; i++) {
			while (!(uart_get(dev, UART_LSR) & UART_LSR_THRE)) {
				; // wait for the xmit fifo available
			}
			uart_set(dev, UART_THR, *buf++);
		}
		ret = len;
	}
	return ret;
}

static struct uart_ops uart_ops = {
	.std = {
		.device_class = &class_uart,
		.init = uart_init,
		.fini = uart_fini,
	},
	.read = uart_read,
	.write = uart_write,
};

DRIVER_EXPORT(uart_16550, &uart_ops.std);

// old uart api
#ifndef UART_16550_NOOLD
static struct device *uart_find(int port)
{
	char name[16];
	sprintf(name, "uart%d", port);

	struct device *dev = device_find(name);
	if (dev) {
		if (uart_init(dev) < 0) {
			dev = NULL;
		}
	}
	return dev;
}

int uart_putc(int port, char c)
{
	int ret = ERR_INVALID_ARGS;
	struct device *dev = uart_find(port);
	if (dev) {
		ret = uart_write(dev, &c, 1);
	}
	return ret;
}

int uart_getc(int port, bool wait)
{
	int ret = ERR_INVALID_ARGS;
	struct device *dev = uart_find(port);
	if (dev) {
		char c;
		struct uart_16550_state *state = dev->state;
		if (cbuf_read_char(&state->rxbuf, &c, wait) == 1) {
			ret = c;
		} else {
			ret = ERR_NOT_READY;
		}
	}
	return ret;
}

void uart_flush_tx(int port)
{
	struct device *dev = uart_find(port);
	if (dev) {
		while (!(uart_get(dev, UART_LSR) & UART_LSR_TEMT)) {
			; // wait for the xmit fifo empty
		}
	}
}

void uart_flush_rx(int port)
{
	while (uart_getc(port, false) >= 0) {
		; // nothing to do
	}
}

void uart_init_port(int port, uint baud)
{
	struct device *dev = uart_find(port);
	if (dev) {
		uart_set_baud(dev, baud);
	}
}
#endif

