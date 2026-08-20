/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/semihosting.h>

#include <lk/err.h>
#include <string.h>

long semihosting_call(ulong op, void *param) {
#if ARM_ISA_ARMV6M || ARM_ISA_ARMV7M || ARM_ISA_ARMV8M
    register ulong r0 __asm__("r0") = op;
    register void *r1 __asm__("r1") = param;

    /*
     * The immediate is part of the instruction encoding and is what the host
     * matches on, so it has to be exactly 0xab here.
     */
    __asm__ volatile("bkpt 0xab" : "+r"(r0) : "r"(r1) : "memory");

    return (long)r0;
#else
#error "semihosting is not implemented for this architecture"
#endif
}

void semihosting_puts(const char *str) {
    semihosting_call(SEMIHOST_SYS_WRITE0, (void *)str);
}

ssize_t semihosting_get_cmdline(char *buf, size_t len) {
    if (len == 0) {
        return ERR_NOT_ENOUGH_BUFFER;
    }

    /*
     * The host reads the buffer and its size out of this block and writes the
     * resulting length back into the second word.
     */
    struct {
        char *buf;
        ulong len;
    } args = { buf, len };

    if (semihosting_call(SEMIHOST_SYS_GET_CMDLINE, &args) != 0) {
        return ERR_NOT_ENOUGH_BUFFER;
    }

    /* Be defensive about what the host wrote back before trusting it. */
    if (args.len >= len) {
        return ERR_NOT_ENOUGH_BUFFER;
    }
    buf[args.len] = '\0';

    return (ssize_t)args.len;
}

void semihosting_exit(ulong reason) {
    /*
     * Unlike most operations the 32 bit form of SYS_EXIT takes the reason code
     * itself in the parameter register rather than a pointer to a block.
     */
    semihosting_call(SEMIHOST_SYS_EXIT, (void *)reason);
}
