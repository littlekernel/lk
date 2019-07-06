/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __DEV_CLASS_UART_H
#define __DEV_CLASS_UART_H

#include <dev/driver.h>

/* uart interface */
struct uart_ops {
    struct driver_ops std;

    ssize_t (*read)(struct device *dev, void *buf, size_t len);
    ssize_t (*write)(struct device *dev, const void *buf, size_t len);
};


ssize_t class_uart_read(struct device *dev, void *buf, size_t len);
ssize_t class_uart_write(struct device *dev, const void *buf, size_t len);

#endif

