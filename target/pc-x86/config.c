/*
 * Copyright (c) 2012 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <dev/driver.h>
#include <dev/class/block.h>
#include <dev/class/netif.h>
#include <platform/uart.h>
#include <platform/ide.h>
#include <platform/pcnet.h>
#include <platform.h>
#include <target.h>
#include <malloc.h>
#include <string.h>
#include <lk/debug.h>

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

// two legacy ISA IDE devices
static const struct platform_ide_config ide0_config = {
    .legacy_index = 0,
};
DEVICE_INSTANCE(ide, ide0, &ide0_config, 0);

static const struct platform_ide_config ide1_config = {
    .legacy_index = 1,
};
DEVICE_INSTANCE(ide, ide1, &ide1_config, 0);

// two PCI IDE devices
static const struct platform_ide_config pci_ide0_config = {
    .legacy_index = 0x80,
};
DEVICE_INSTANCE(ide, pci_ide0, &pci_ide0_config, 0);

static const struct platform_ide_config pci_ide1_config = {
    .legacy_index = 0x81,
};
DEVICE_INSTANCE(ide, pci_ide1, &pci_ide1_config, 0);

void target_init(void) {
    // initialize static devices
    device_init_all();

    // try to initialize pci ide first
    if (device_init(&__device_ide_pci_ide0) < 0 || device_init(&__device_ide_pci_ide1) < 0) {
        // if that fails, initialize the legacy ISA IDE controllers
        device_init(&__device_ide_ide0);
        device_init(&__device_ide_ide1);
    }

}

