/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <reg.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <sys/types.h>
#include <target/qemu-microblaze.h>

lk_bigtime_t current_time_hires(void)
{
    return 0;
}

lk_time_t current_time(void)
{
    return 0;
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
    return 0;
}

static void _dputc(char c)
{
    *REG32(UARTLITE_BASEADDR + 0x4) = c;
}

void platform_dputc(char c)
{
    if (c == '\n')
        _dputc('\r');
    _dputc(c);
}

int platform_dgetc(char *c, bool wait)
{
#if 0
    int ret = uart_getc(DEBUG_UART, wait);
    if (ret == -1)
        return -1;
    *c = ret;
    return 0;
#endif
    return -1;

}


