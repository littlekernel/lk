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
#include <ffs.h>

#include <lwip/tcpip.h>

#define LOCAL_TRACE 1

static const struct platform_uart_config uart0_config = {
	.io_port = 0x3f8,
	.irq = 0x24,
	.baud_rate = 115200,
	.rx_buf_len = 1024,
	.tx_buf_len = 1024,
};

DEVICE_INSTANCE(uart, uart0, &uart0_config);

static const struct platform_ide_config ide0_config = {
};

DEVICE_INSTANCE(ide, ide0, &ide0_config);

static const struct platform_pcnet_config pcnet0_config = {
	.vendor_id = 0x1022,
	.device_id = 0x2000,
	.index = 0,
};

DEVICE_INSTANCE(netif, pcnet0, &pcnet0_config);

void target_init(void) {
	//device_init_all();

	device_init(device_get_by_name(ide, ide0));
	ffs_mount(0, device_get_by_name(ide, ide0));

	tcpip_init(NULL, NULL);

	device_init(device_get_by_name(netif, pcnet0));
	class_netif_add(device_get_by_name(netif, pcnet0));
}

