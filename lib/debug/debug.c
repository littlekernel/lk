/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
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

#include <ctype.h>
#include <debug.h>
#include <stdlib.h>
#include <printf.h>
#include <stdio.h>
#include <list.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/spinlock.h>

void spin(uint32_t usecs)
{
    lk_bigtime_t start = current_time_hires();

    while ((current_time_hires() - start) < usecs)
        ;
}

void _panic(void *caller, const char *fmt, ...)
{
    printf("panic (caller %p): ", caller);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

#if !DISABLE_DEBUG_OUTPUT

static int __panic_stdio_fgetc(void *ctx)
{
    char c;
    int err;

    err = platform_pgetc(&c, false);
    if (err < 0)
        return err;
    return (unsigned char)c;
}

static ssize_t __panic_stdio_read(io_handle_t *io, char *s, size_t len)
{
    if (len == 0)
        return 0;

    int err = platform_pgetc(s, false);
    if (err < 0)
        return err;

    return 1;
}

static ssize_t __panic_stdio_write(io_handle_t *io, const char *s, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        platform_pputc(s[i]);
    }
    return len;
}

FILE *get_panic_fd(void)
{
    static const io_handle_hooks_t panic_hooks = {
        .write = __panic_stdio_write,
        .read = __panic_stdio_read,
    };
    static io_handle_t panic_io = {
        .magic = IO_HANDLE_MAGIC,
        .hooks = &panic_hooks
    };
    static FILE panic_fd = {
        .io = &panic_io
    };

    return &panic_fd;
}

void hexdump(const void *ptr, size_t len)
{
    addr_t address = (addr_t)ptr;
    size_t count;

    for (count = 0 ; count < len; count += 16) {
        union {
            uint32_t buf[4];
            uint8_t  cbuf[16];
        } u;
        size_t s = ROUNDUP(MIN(len - count, 16), 4);
        size_t i;

        printf("0x%08lx: ", address);
        for (i = 0; i < s / 4; i++) {
            u.buf[i] = ((const uint32_t *)address)[i];
            printf("%08x ", u.buf[i]);
        }
        for (; i < 4; i++) {
            printf("         ");
        }
        printf("|");

        for (i=0; i < 16; i++) {
            char c = u.cbuf[i];
            if (i < s && isprint(c)) {
                printf("%c", c);
            } else {
                printf(".");
            }
        }
        printf("|\n");
        address += 16;
    }
}

void hexdump8_ex(const void *ptr, size_t len, uint64_t disp_addr)
{
    addr_t address = (addr_t)ptr;
    size_t count;
    size_t i;
    const char *addr_fmt = ((disp_addr + len) > 0xFFFFFFFF)
                           ? "0x%016llx: "
                           : "0x%08llx: ";

    for (count = 0 ; count < len; count += 16) {
        printf(addr_fmt, disp_addr + count);

        for (i=0; i < MIN(len - count, 16); i++) {
            printf("%02hhx ", *(const uint8_t *)(address + i));
        }

        for (; i < 16; i++) {
            printf("   ");
        }

        printf("|");

        for (i=0; i < MIN(len - count, 16); i++) {
            char c = ((const char *)address)[i];
            printf("%c", isprint(c) ? c : '.');
        }

        printf("\n");
        address += 16;
    }
}

#endif // !DISABLE_DEBUG_OUTPUT
