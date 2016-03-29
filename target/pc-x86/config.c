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

#include <dev/driver.h>
#include <dev/class/block.h>
#include <dev/class/netif.h>
#include <platform/uart.h>
#include <platform/ide.h>
#include <platform/pcnet.h>
#include <platform.h>
#include <malloc.h>
#include <string.h>
#include <debug.h>

#if 0
static const struct platform_uart_config uart0_config = {
    .io_port = 0x3f8,
    .irq = 0x24,
    .baud_rate = 115200,
    .rx_buf_len = 1024,
    .tx_buf_len = 1024,
};

DEVICE_INSTANCE(uart, uart0, &uart0_config);
#endif

#ifndef ARCH_X86_64
static const struct platform_ide_config ide0_config = {
};

DEVICE_INSTANCE(ide, ide0, &ide0_config);

#endif

void target_init(void)
{
    //device_init_all();
}

