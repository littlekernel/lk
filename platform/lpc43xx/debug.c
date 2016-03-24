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
#include <kernel/thread.h>
#include <lib/cbuf.h>

#include <arch/arm/cm.h>
#include <platform/lpc43xx-uart.h>
#include <platform/lpc43xx-clocks.h>

static cbuf_t console_rx_buf;

#ifndef TARGET_DEBUG_BAUDRATE
#define TARGET_DEBUG_BAUDRATE 115200
#endif

#if TARGET_DEBUG_UART == 1
#define UART_BASE UART0_BASE
#define UART_IRQ lpc43xx_USART0_IRQ
#define UART_IRQn USART0_IRQn
#elif TARGET_DEBUG_UART == 2
#define UART_BASE UART1_BASE
#define UART_IRQ lpc43xx_UART1_IRQ
#define UART_IRQn UART1_IRQn
#elif TARGET_DEBUG_UART == 3
#define UART_BASE UART2_BASE
#define UART_IRQ lpc43xx_USART2_IRQ
#define UART_IRQn USART2_IRQn
#elif TARGET_DEBUG_UART == 4
#define UART_BASE UART3_BASE
#define UART_IRQ lpc43xx_USART3_IRQ
#define UART_IRQn USART3_IRQn
#else
#warning TARGET_DEBUG_UART unspecified
#endif

static u32 base_uart_clk[4] = {
    BASE_UART0_CLK,
    BASE_UART1_CLK,
    BASE_UART2_CLK,
    BASE_UART3_CLK
};

extern uint8_t __lpc43xx_main_clock_sel;
extern uint32_t __lpc43xx_main_clock_mhz;

#define ITM_STIM0   0xE0000000
#define ITM_TER     0xE0000E00
#define ITM_TCR     0xE0000E80
#define ITM_LAR     0xE0000FB0

#define TPI_ACPR    0xE0040010
#define TPI_SPPR    0xE00400F0
#define TPI_FFCR    0xE0040304

#define DEMCR       0xE000EDFC
#define DEMCR_TRCENA    (1 << 24)

void lpc43xx_debug_early_init(void)
{
    // ensure ITM and DWT are enabled
    writel(readl(DEMCR) | DEMCR_TRCENA, DEMCR);

    writel((1 << 9) | (1 << 16) | (2 << 10), DWT_CTRL);

    // configure TPIU for one-wire, nrz, 6mbps
    writel((__lpc43xx_main_clock_mhz / 6000000) - 1, TPI_ACPR);
    writel(2, TPI_SPPR);
    writel(0x100, TPI_FFCR);

    // configure ITM
    writel(0xC5ACCE55, ITM_LAR); // unlock regs
    writel(0x0001000D, ITM_TCR); // ID=1, enable ITM, SYNC, DWT events
    writel(0xFFFFFFFF, ITM_TER); // enable all trace ports

#ifdef UART_BASE
#if TARGET_DEBUG_BAUDRATE == 115200
    // config for 115200-n-8-1 from 12MHz clock
    writel(BASE_CLK_SEL(CLK_IRC), base_uart_clk[TARGET_DEBUG_UART - 1]);
    writel(LCR_DLAB, UART_BASE + REG_LCR);
    writel(4, UART_BASE + REG_DLL);
    writel(0, UART_BASE + REG_DLM);
    writel(FDR_DIVADDVAL(5) | FDR_MULVAL(8), UART_BASE + REG_FDR);
#else
    uint32_t div = __lpc43xx_main_clock_mhz / 16 / TARGET_DEBUG_BAUDRATE;
    writel(BASE_CLK_SEL(__lpc43xx_main_clock_sel),
           base_uart_clk[TARGET_DEBUG_UART - 1]);
    writel(LCR_DLAB, UART_BASE + REG_LCR);
    writel(div & 0xFF, UART_BASE + REG_DLL);
    writel((div >> 8) & 0xFF, UART_BASE + REG_DLM);
#endif
    writel(LCR_WLS_8 | LCR_SBS_1, UART_BASE + REG_LCR);
    writel(FCR_FIFOEN | FCR_RX_TRIG_1, UART_BASE + REG_FCR);
    writel(IER_RBRIE, UART_BASE + REG_IER);
    NVIC_EnableIRQ(UART_IRQn);
#endif
}

void lpc43xx_debug_init(void)
{
    cbuf_initialize(&console_rx_buf, 64);
}

#ifdef UART_BASE
void UART_IRQ (void)
{
    arm_cm_irq_entry();
    while (readl(UART_BASE + REG_LSR) & LSR_RDR) {
        unsigned c = readl(UART_BASE + REG_RBR);
        if (cbuf_space_avail(&console_rx_buf)) {
            cbuf_write_char(&console_rx_buf, c, false);
        }
    }
    arm_cm_irq_exit(1);
}
#endif

void platform_dputc(char c)
{
    // if ITM is enabled, send character to STIM0
    if (readl(ITM_TCR) & 1) {
        while (!readl(ITM_STIM0)) ;
        writeb(c, ITM_STIM0);
    }
#ifdef UART_BASE
    while (!(readl(UART_BASE + REG_LSR) & LSR_THRE)) ;
    writel(c, UART_BASE + REG_THR);
#endif
}

int platform_dgetc(char *c, bool wait)
{
    if (cbuf_read_char(&console_rx_buf, c, wait) == 0)
        return -1;
    return 0;
}

#define DCRDR 0xE000EDF8

void _debugmonitor(void)
{
    u32 n;
    arm_cm_irq_entry();
    n = readl(DCRDR);
    if (n & 0x80000000) {
        switch (n >> 24) {
            case 0x80: // write to console
                if (cbuf_space_avail(&console_rx_buf)) {
                    cbuf_write_char(&console_rx_buf, n & 0xFF, false);
                }
                n = 0;
                break;
            default:
                n = 0x01000000;
        }
        writel(n, DCRDR);
    }
    arm_cm_irq_exit(1);
}
