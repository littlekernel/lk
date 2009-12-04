/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <printf.h>
#include <platform/at91sam7.h>
#include <kernel/thread.h>
#include <stdarg.h>

void ser_init(void)
{
    AT91DBGU *dbgu = AT91DBGU_ADDR;

//    AT91PIO *pio = AT91PIO_ADDR;
//    pio->select_a = PIN_DRXD | PIN_DTXD;
//    pio->pio_disable = PIN_DRXD | PIN_DTXD;
    
    dbgu->MR = DBGU_PAR_NONE | DBGU_MODE_NORMAL;
    // dbgu->BRGR = 10; //MCK_IN_MHZ / 115200 / 16; 
    dbgu->BRGR = AT91_MCK_MHZ / 115200 / 16;
    dbgu->CR = DBGU_RXEN | DBGU_TXEN;
}

void ser_putc(unsigned c)
{
    AT91DBGU *dbgu = AT91DBGU_ADDR;
    if(c == 10) {
        while(!(dbgu->SR & DBGU_TXRDY));
        dbgu->THR = 13;
    }
    while(!(dbgu->SR & DBGU_TXRDY));
    dbgu->THR = c;
}

void ser_puts(const char *s)
{
    AT91DBGU *dbgu = AT91DBGU_ADDR;
    while(*s) {
        if(*s == 10) {
            while(!(dbgu->SR & DBGU_TXRDY));
            dbgu->THR = 13;
        }
        while(!(dbgu->SR & DBGU_TXRDY));
        dbgu->THR = *s++;
    }
}

int dgetc(char *c, bool wait)
{
	return -1;
}


void _dputc(char c)
{
	ser_putc(c);
}

void platform_halt()
{
	arch_disable_ints();
    for(;;);
}

