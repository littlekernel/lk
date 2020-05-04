/*
 * Copyright (c) 2019 Elliot Berman
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <config.h>

#if !defined(RISCV_M_MODE) || !(RISCV_M_MODE)

struct sbiret {
    long error;
    long value;
};

enum sbi_return_code {
    SBI_SUCCESS = 0,
    SBI_ERR_FAILURE = -1,
    SBI_ERR_NOT_SUPPORTED = -2,
    SBI_ERR_INVALID_PARAM = -3,
    SBI_ERR_DENIED = -4,
    SBI_ERR_INVALID_ADDRESS = -5,
};

// make a SBI call according to the SBI spec at https://github.com/riscv/riscv-sbi-doc
// Note: it seems ambigious whether or not a2-a7 are trashed in the call, but the
// OpenSBI and linux implementations seem to assume that all of the regs are restored
// aside from a0 and a1 which are used for return values.
#define _sbi_call(extension, function, arg0, arg1, arg2, arg3, arg4, arg5, ...) ({  \
    register unsigned long a0 asm("a0") = (unsigned long)arg0;      \
    register unsigned long a1 asm("a1") = (unsigned long)arg1;      \
    register unsigned long a2 asm("a2") = (unsigned long)arg2;      \
    register unsigned long a3 asm("a3") = (unsigned long)arg3;      \
    register unsigned long a4 asm("a4") = (unsigned long)arg4;      \
    register unsigned long a5 asm("a5") = (unsigned long)arg5;      \
    register unsigned long a6 asm("a6") = (unsigned long)function;      \
    register unsigned long a7 asm("a7") = (unsigned long)extension;     \
    asm volatile ("ecall"                       \
        : "+r" (a0), "+r" (a1)                  \
        : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r"(a6), "r"(a7) \
        : "memory");                        \
    struct sbiret ret = { .error = a0, .value = a1 };       \
    ret;                                \
    })
#define sbi_call(...) \
    _sbi_call(__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0)


#define SBI_SET_TIMER               0x00, 0
#define SBI_CONSOLE_PUTCHAR         0x01, 0
#define SBI_CONSOLE_GETCHAR         0x02, 0
#define SBI_CLEAR_IPI               0x03, 0
#define SBI_SEND_IPI                0x04, 0
#define SBI_REMOTE_FENCEI           0x05, 0
#define SBI_REMOTE_SFENCE_VMA       0x06, 0
#define SBI_REMOTE_SFENCE_VMA_ASID  0x07, 0
#define SBI_SHUTDOWN                0x08, 0

#define SBI_GET_SBI_SPEC_VERSION    0x10, 0
#define SBI_GET_SBI_IMPL_ID         0x10, 1
#define SBI_GET_SBI_IMPL_VERSION    0x10, 2
#define SBI_PROBE_EXTENSION         0x10, 3
#define SBI_GET_MVENDORID           0x10, 4
#define SBI_GET_MARCHID             0x10, 5
#define SBI_GET_MIMPID              0x10, 6

#define SBI_EXT_TIMER               0x54494d45
#define SBI_EXT_IPI                 0x00735049
#define SBI_EXT_RFENCE              0x52464e43
#define SBI_EXT_HSM                 0x0048534d

static inline void sbi_set_timer(uint64_t stime_value) {
    sbi_call(SBI_SET_TIMER, stime_value);
}

static inline void sbi_send_ipis(const unsigned long *hart_mask) {
    sbi_call(SBI_SEND_IPI, hart_mask);
}

static inline void sbi_clear_ipi(void) {
    sbi_call(SBI_CLEAR_IPI);
}

# if SUBARCH == 32
static inline uint64_t riscv_get_time(void) {
    uint32_t hi, lo;

    do {
        hi = riscv_csr_read(RISCV_CSR_TIMEH);
        lo = riscv_csr_read(RISCV_CSR_TIME);
    } while (hi != riscv_csr_read(RISCV_CSR_TIMEH));

    return (((uint64_t)hi << 32) | lo);
}
# else
static inline uint64_t riscv_get_time(void) {
    return riscv_csr_read(RISCV_CSR_TIME);
}
# endif

#endif
