/*
 * Copyright (c) 2015 Brian Swetland
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

#include <debug.h>
#include <reg.h>

#include <platform/lpc43xx-uart.h>
#include <platform/lpc43xx-clocks.h>

#ifndef TARGET_DEBUG_BAUDRATE
#define TARGET_DEBUG_BAUDRATE 115200
#endif

#if TARGET_DEBUG_UART == 1
#define UART_BASE UART0_BASE
#elif TARGET_DEBUG_UART == 2
#define UART_BASE UART1_BASE
#elif TARGET_DEBUG_UART == 3
#define UART_BASE UART2_BASE
#elif TARGET_DEBUG_UART == 4
#define UART_BASE UART3_BASE
#else
#warning TARGET_DEBUG_UART unspecified
#endif

static u32 base_uart_clk[4] = {
	BASE_UART0_CLK,
	BASE_UART1_CLK,
	BASE_UART2_CLK,
	BASE_UART3_CLK
};

void lpc43xx_debug_early_init(void)
{
#ifdef UART_BASE
#if TARGET_DEBUG_BAUDRATE == 115200
	// config for 115200-n-8-1 from 12MHz clock
	writel(BASE_X_SEL(CLK_IRC), base_uart_clk[TARGET_DEBUG_UART - 1]);
	writel(LCR_DLAB, UART_BASE + REG_LCR);
	writel(4, UART_BASE + REG_DLL);
	writel(0, UART_BASE + REG_DLM);
	writel(FDR_DIVADDVAL(5) | FDR_MULVAL(8), UART_BASE + REG_FDR);
#else
	writel(BASE_X_SEL(CLK_IDIVC), base_uart_clk[TARGET_DEBUG_UART - 1]);
	writel(LCR_DLAB, UART_BASE + REG_LCR);
#if TARGET_DEBUG_BAUDRATE == 1000000
	writel(6, UART_BASE + REG_DLL);
#elif TARGET_DEBUG_BAUDRATE == 2000000
	writel(3, UART_BASE + REG_DLL);
#elif TARGET_DEBUG_BAUDRATE == 3000000
	writel(2, UART_BASE + REG_DLL);
#else
#error Unsupported TARGET_DEBUG_BAUDRATE
#endif
	writel(0, UART_BASE + REG_DLM);
	writel(0, UART_BASE + REG_FDR);
#endif
	writel(LCR_WLS_8 | LCR_SBS_1, UART_BASE + REG_LCR);
	writel(FCR_FIFOEN | FCR_RX_TRIG_1, UART_BASE + REG_FCR);
#endif
}

void lpc43xx_debug_init(void)
{
}

void platform_dputc(char c)
{
#ifdef UART_BASE
	while (!(readl(UART_BASE + REG_LSR) & LSR_THRE)) ;
	writel(c, UART_BASE + REG_THR);
#endif
}

int platform_dgetc(char *c, bool wait)
{
#ifdef UART_BASE
	while (!(readl(UART_BASE + REG_LSR) & LSR_RDR)) {
		if (!wait) {
			return -1;
		}
	}

	*c = readl(UART_BASE + REG_RBR);
	return 0;
#else
	if (wait) {
		for (;;) ;
	}
	return -1;
#endif
}

