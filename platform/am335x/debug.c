/*
 * Copyright (c) 2012 Corey Tabaka
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
#include <arch/ops.h>
#include <debug.h>
#include <reg.h>
#include <compiler.h>
#include <lib/cbuf.h>
#include <platform/interrupts.h>

#include <kernel/thread.h>

#include <hw_control_AM335x.h>
#include <soc_AM335x.h>
#include <hw_cm_wkup.h>
#include <hw_cm_per.h>
#include <hw_types.h>
#include <hw_uart_irda_cir.h>
#include <uart_irda_cir.h>
#include <interrupt.h>

#define UART_CONSOLE_BASE                    (SOC_UART_0_REGS)
#define BAUD_RATE_115200                     (115200)
#define UART_MODULE_INPUT_CLK                (48000000)
#define UART_CONSOLE_INT                     (SYS_INT_UART0INT)

static cbuf_t uart_rx_buf;

static enum handler_return uart_irq_handler(void *arg)
{
	unsigned char c;
	unsigned int lcr;
	bool resched = false;

	lcr = UARTRegConfigModeEnable(UART_CONSOLE_BASE, UART_REG_OPERATIONAL_MODE);

	while (HWREG(UART_CONSOLE_BASE + UART_LSR) & UART_LSR_RX_FIFO_E) {
		c = (char) HWREG(UART_CONSOLE_BASE + UART_RHR);
		cbuf_write_char(&uart_rx_buf, c, false);
		resched = true;
	}

	HWREG(UART_CONSOLE_BASE + UART_LCR) = lcr;

	return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

void uart_init(void)
{
	/* finish uart init to get rx going */
	cbuf_initialize(&uart_rx_buf, 16);

	register_int_handler(UART_CONSOLE_INT, uart_irq_handler, NULL);
	unmask_interrupt(UART_CONSOLE_INT);

	/* enable RHR/CTI interrupt */
	UARTIntEnable(UART_CONSOLE_BASE, UART_INT_RHR_CTI);
}

static void uart_putc(char c)
{
	UARTCharPut(UART_CONSOLE_BASE, c);
}

static int uart_getc(char *c, bool wait)
{
	return cbuf_read_char(&uart_rx_buf, c, wait);
}

void platform_dputc(char c)
{
	if (c == '\n')
		uart_putc('\r');
	uart_putc(c);
}

int platform_dgetc(char *c, bool wait)
{
	return uart_getc(c, wait);
}

void platform_init_debug(void)
{
	/* configure UART0 clock */
	HWREG(SOC_CM_WKUP_REGS + CM_WKUP_UART0_CLKCTRL) |=
	    CM_WKUP_UART0_CLKCTRL_MODULEMODE_ENABLE;

	while (CM_WKUP_UART0_CLKCTRL_MODULEMODE_ENABLE !=
	        (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_UART0_CLKCTRL) &
	         CM_WKUP_UART0_CLKCTRL_MODULEMODE));

	while (CM_WKUP_CLKSTCTRL_CLKACTIVITY_UART0_GFCLK !=
	        (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_CLKSTCTRL) &
	         CM_WKUP_CLKSTCTRL_CLKACTIVITY_UART0_GFCLK));

	while ((CM_WKUP_UART0_CLKCTRL_IDLEST_FUNC << CM_WKUP_UART0_CLKCTRL_IDLEST_SHIFT) !=
	        (HWREG(SOC_CM_WKUP_REGS + CM_WKUP_UART0_CLKCTRL) &
	         CM_WKUP_UART0_CLKCTRL_IDLEST));

	/* RXD */
	HWREG(SOC_CONTROL_REGS + CONTROL_CONF_UART_RXD(0)) =
	    (CONTROL_CONF_UART0_RXD_CONF_UART0_RXD_PUTYPESEL |
	     CONTROL_CONF_UART0_RXD_CONF_UART0_RXD_RXACTIVE);

	/* TXD */
	HWREG(SOC_CONTROL_REGS + CONTROL_CONF_UART_TXD(0)) =
	    CONTROL_CONF_UART0_TXD_CONF_UART0_TXD_PUTYPESEL;

	/* software reset */
	HWREG(UART_CONSOLE_BASE + UART_SYSC) |= (UART_SYSC_SOFTRESET);
	while (!(HWREG(UART_CONSOLE_BASE + UART_SYSS) & UART_SYSS_RESETDONE));

	/* setup fifo */
	UARTFIFOConfig(UART_CONSOLE_BASE,
	               UART_FIFO_CONFIG(UART_TRIG_LVL_GRANULARITY_1,
	                                UART_TRIG_LVL_GRANULARITY_1, 1, 1, 1, 1,
	                                UART_DMA_EN_PATH_SCR,
	                                UART_DMA_MODE_0_ENABLE));

	/* baud rate settings */
	unsigned int divisor = UARTDivisorValCompute(UART_MODULE_INPUT_CLK,
	                       BAUD_RATE_115200,
	                       UART16x_OPER_MODE,
	                       UART_MIR_OVERSAMPLING_RATE_42);

	UARTDivisorLatchWrite(UART_CONSOLE_BASE, divisor);
	UARTRegConfigModeEnable(UART_CONSOLE_BASE, UART_REG_CONFIG_MODE_B);
	UARTLineCharacConfig(UART_CONSOLE_BASE,
	                     (UART_FRAME_WORD_LENGTH_8 | UART_FRAME_NUM_STB_1),
	                     UART_PARITY_NONE);

	UARTDivisorLatchDisable(UART_CONSOLE_BASE);
	UARTBreakCtl(UART_CONSOLE_BASE, UART_BREAK_COND_DISABLE);
	UARTOperatingModeSelect(UART_CONSOLE_BASE, UART16x_OPER_MODE);
}

void debug_point(char c)
{
	uart_putc('\r');
	uart_putc('\n');
	uart_putc(c);
}

void debug_hex(int val)
{
	unsigned int i, temp;

	uart_putc('\r');
	uart_putc('\n');

	for (i=0; i < 8; i++) {
		temp = (val >> (28 - i*4)) & 0xf;
		uart_putc((temp < 10 ? '0' + temp : 'a' + temp - 10));
	}
}

