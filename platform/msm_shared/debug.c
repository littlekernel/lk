/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <printf.h>
#include <arch/arm/dcc.h>
#include <dev/fbcon.h>
#include <dev/uart.h>

void _dputc(char c)
{
#if WITH_DEBUG_DCC
	if (c == '\n') {
		while (dcc_putc('\r') < 0);
	}
	while (dcc_putc(c) < 0);
#endif
#if WITH_DEBUG_UART
	uart_putc(0, c);
#endif
#if WITH_DEBUG_FBCON && WITH_DEV_FBCON
	fbcon_putc(c);
#endif
}

int dgetc(char *c, bool wait)
{
	int n;
#if WITH_DEBUG_DCC
	n = dcc_getc();
#elif WITH_DEBUG_UART
	n = uart_getc(0, 0);
#else
	n = -1;
#endif
	if (n < 0) {
		return -1;
	} else {
		*c = n;
		return 0;
	}
}

void platform_halt(void)
{
	dprintf(INFO, "HALT: spinning forever...\n");
	for(;;);
}

