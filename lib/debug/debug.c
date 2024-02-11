/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <ctype.h>
#include <lk/debug.h>
#include <stdlib.h>
#include <printf.h>
#include <stdio.h>
#include <assert.h>
#include <lk/list.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/spinlock.h>

void spin(uint32_t usecs) {
    lk_bigtime_t start = current_time_hires();

    while ((current_time_hires() - start) < usecs)
        ;
}

void panic(const char *fmt, ...) {
    printf("panic (caller %p): ", __GET_CALLER());

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void assert_fail_msg(const char* file, int line, const char* expression, const char* fmt, ...) {

    // Print the user message.
    printf("ASSERT FAILED at (%s:%d): %s\n", file, line, expression);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void assert_fail(const char* file, int line, const char* expression) {
    printf("ASSERT FAILED at (%s:%d): %s\n", file, line, expression);
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

#if !DISABLE_DEBUG_OUTPUT

static ssize_t __panic_stdio_read(io_handle_t *io, char *s, size_t len) {
    if (len == 0)
        return 0;

    int err = platform_pgetc(s, false);
    if (err < 0)
        return err;

    return 1;
}

static ssize_t __panic_stdio_write(io_handle_t *io, const char *s, size_t len) {
    for (size_t i = 0; i < len; i++) {
        platform_pputc(s[i]);
    }
    return len;
}

FILE *get_panic_fd(void) {
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

void hexdump(const void *ptr, size_t len) {
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

void hexdump8_ex(const void *ptr, size_t len, uint64_t disp_addr) {
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
