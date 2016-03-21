/*
 * Copyright (c) 2015 MediaTek Inc.
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
#include <dev/uart.h>
#include <string.h>

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_uart.h>
#include <sync_write.h>

// output uart port
static volatile unsigned int g_uart;
// output uart baudrate
static unsigned int g_brg;

static void uart_setbrg(void)
{
    unsigned int byte,speed;
    unsigned int highspeed;
    unsigned int quot, divisor, remainder;
    unsigned int uartclk;
    unsigned short data, high_speed_div, sample_count, sample_point;
    unsigned int tmp_div;

    speed = g_brg;
    uartclk = UART_SRC_CLK;

    if (speed <= 115200 ) {
        highspeed = 0;
        quot = 16;
    } else {
        highspeed = 3;
        quot = 1;
    }

    if (highspeed < 3) { /*0~2*/
        /* Set divisor DLL and DLH  */
        divisor   =  uartclk / (quot * speed);
        remainder =  uartclk % (quot * speed);

        if (remainder >= (quot / 2) * speed)
            divisor += 1;

        mt_reg_sync_writew(highspeed, UART_HIGHSPEED(g_uart));
        byte = DRV_Reg32(UART_LCR(g_uart));   /* DLAB start */
        mt_reg_sync_writel((byte | UART_LCR_DLAB), UART_LCR(g_uart));
        mt_reg_sync_writel((divisor & 0x00ff), UART_DLL(g_uart));
        mt_reg_sync_writel(((divisor >> 8)&0x00ff), UART_DLH(g_uart));
        mt_reg_sync_writel(byte, UART_LCR(g_uart));   /* DLAB end */
    } else {
        data=(unsigned short)(uartclk/speed);
        high_speed_div = (data>>8) + 1; // divided by 256

        tmp_div=uartclk/(speed*high_speed_div);
        divisor =  (unsigned short)tmp_div;

        remainder = (uartclk)%(high_speed_div*speed);
        /*get (sample_count+1)*/
        if (remainder >= ((speed)*(high_speed_div))>>1)
            divisor =  (unsigned short)(tmp_div+1);
        else
            divisor =  (unsigned short)tmp_div;

        sample_count=divisor-1;

        /*get the sample point*/
        sample_point=(sample_count-1)>>1;

        /*configure register*/
        mt_reg_sync_writel(highspeed, UART_HIGHSPEED(g_uart));

        byte = DRV_Reg32(UART_LCR(g_uart));    /* DLAB start */
        mt_reg_sync_writel((byte | UART_LCR_DLAB), UART_LCR(g_uart));
        mt_reg_sync_writel((high_speed_div & 0x00ff), UART_DLL(g_uart));
        mt_reg_sync_writel(((high_speed_div >> 8)&0x00ff), UART_DLH(g_uart));
        mt_reg_sync_writel(sample_count, UART_SAMPLE_COUNT(g_uart));
        mt_reg_sync_writel(sample_point, UART_SAMPLE_POINT(g_uart));
        mt_reg_sync_writel(byte, UART_LCR(g_uart));   /* DLAB end */
    }
}

void mtk_set_current_uart(MTK_UART uart_base)
{
    g_uart = uart_base;
}

int mtk_get_current_uart(void)
{
    return g_uart;
}

void uart_init_early(void)
{
    mtk_set_current_uart(UART1);

    DRV_SetReg32(UART_FCR(g_uart), UART_FCR_FIFO_INIT); /* clear fifo */
    mt_reg_sync_writew(UART_NONE_PARITY | UART_WLS_8 | UART_1_STOP, UART_LCR(g_uart));
    g_brg = CONFIG_BAUDRATE;
    uart_setbrg();
}

void uart_init(void)
{
}

void uart_flush_tx(int port)
{
}

void uart_flush_rx(int port)
{
}

void uart_init_port(int port, uint baud)
{
}

int uart_putc(int port, char c)
{
    while (!(DRV_Reg32(UART_LSR(port)) & UART_LSR_THRE));

    if (c == '\n')
        mt_reg_sync_writel((unsigned int)'\r', UART_THR(port));

    mt_reg_sync_writel((unsigned int)c, UART_THR(port));

    return 0;
}

int uart_getc(int port, bool wait)
{
    while (!(DRV_Reg32(UART_LSR(port)) & UART_LSR_DR));
    return (int)DRV_Reg32(UART_RBR(port));
}

