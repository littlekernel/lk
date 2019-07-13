/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <sys/types.h>

void uart_init(void);
void uart_init_early(void);

int uart_putc(int port, char c);
int uart_getc(int port, bool wait);
void uart_flush_tx(int port);
void uart_flush_rx(int port);
void uart_init_port(int port, uint baud);

/* panic-time uart accessors, intended to be run with interrupts disabled */
int uart_pputc(int port, char c);
int uart_pgetc(int port);

