/*
 * Copyright (c) 2012 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

struct platform_uart_config {
    int baud_rate;
    int io_port;
    int irq;
    int rx_buf_len;
    int tx_buf_len;
};

