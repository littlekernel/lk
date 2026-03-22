// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <lk/reg.h>
#include <lib/cbuf.h>

#if 0
void uart_init_early(void) {
	for (;;) ;
}

void uart_init(void) {
}

int uart_putc(int port, char c) {
	return 1;
}

int uart_getc(int port, bool wait) {
    return -1;
}

void uart_flush_tx(int port) {}

void uart_flush_rx(int port) {}

void uart_init_port(int port, uint baud) {}

#endif
