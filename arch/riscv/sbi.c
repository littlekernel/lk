/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/riscv/sbi.h>

#include <stdbool.h>
#include <stdio.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <arch/riscv.h>

#include "riscv_priv.h"

#if RISCV_S_MODE

// bitmap of locally detected SBI extensions
enum sbi_extension {
    SBI_EXTENSION_TIMER = 1,
    SBI_EXTENSION_IPI,
    SBI_EXTENSION_RFENCE,
    SBI_EXTENSION_HSM,
    SBI_EXTENSION_SRST,
    SBI_EXTENSION_PMU,
    SBI_EXTENSION_DBCN,
    SBI_EXTENSION_SUSP,
    SBI_EXTENSION_CPPC,
    SBI_EXTENSION_NACL,
    SBI_EXTENSION_STA,
};

static uint sbi_ext;

// make a SBI call according to the SBI spec at https://github.com/riscv/riscv-sbi-doc
// args are passed a0-a5, a6 holds the function id, a7 holds the extension id
// return struct sbiret in a0, a1.
// all registers except for a0 and a1 are preserved.
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
    (struct sbiret){ .error = a0, .value = a1 };       \
    })
#define sbi_call(...) \
    _sbi_call(__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0)

static inline bool sbi_ext_present(enum sbi_extension e) {
    return sbi_ext & (1 << e);
}

struct sbiret sbi_generic_call_2(ulong extension, ulong function) {
    return sbi_call(extension, function);
}

bool sbi_probe_extension(ulong extension) {
    return sbi_call(SBI_PROBE_EXTENSION, extension).value != 0;
}

void sbi_set_timer(uint64_t stime_value) {
    // use the new IPI extension
    if (likely(sbi_ext_present(SBI_EXTENSION_TIMER))) {
        sbi_call(SBI_EXT_TIMER_SIG, 0, stime_value);
    } else {
        sbi_call(SBI_SET_TIMER, stime_value);
    }
}

void sbi_send_ipis(const unsigned long *hart_mask) {
    // use the new IPI extension
    if (likely(sbi_ext_present(SBI_EXTENSION_IPI))) {
        sbi_call(SBI_EXT_IPI_SIG, 0, *hart_mask, -1);
    } else {
        // legacy ipi call
        sbi_call(SBI_SEND_IPI, hart_mask);
    }
}

void sbi_clear_ipi(void) {
    // deprecated, clear sip.SSIP directly
    riscv_csr_clear(RISCV_CSR_XIP, RISCV_CSR_XIP_SIP);
    //sbi_call(SBI_CLEAR_IPI);
}

status_t sbi_boot_hart(uint hartid, paddr_t start_addr, ulong arg) {
    if (!sbi_ext_present(SBI_EXTENSION_HSM))
        return ERR_NOT_IMPLEMENTED;

    // try to use the HSM implementation to boot a cpu
    struct sbiret ret = sbi_call(SBI_EXT_HSM_SIG, 0, hartid, start_addr, arg);
    if (ret.error < 0) {
        return ERR_INVALID_ARGS;
    }

    return NO_ERROR;
}

void sbi_rfence_vma(const unsigned long *hart_mask, vaddr_t vma, size_t size) {
    // use the new IPI extension
    if (likely(sbi_ext_present(SBI_EXTENSION_RFENCE))) {
        sbi_call(SBI_EXT_RFENCE_SIG, 1, *hart_mask, 0, vma, size);
    } else {
        PANIC_UNIMPLEMENTED;
    }
}

status_t sbi_system_reset(uint32_t type, uint32_t reason) {
    if (likely(sbi_ext_present(SBI_EXTENSION_SRST))) {
        ulong error = sbi_call(SBI_EXT_SRST_SIG, 0, type, reason).error;
        if (error != 0) {
            return ERR_GENERIC;
        }
        // we really shouldn't get here
        return NO_ERROR;
    } else {
        return ERR_NOT_IMPLEMENTED;
    }
}

void sbi_early_init(void) {
    // read the presence of some features
    sbi_ext |= sbi_probe_extension(SBI_EXT_TIMER_SIG) ? (1<<SBI_EXTENSION_TIMER) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_IPI_SIG) ? (1<<SBI_EXTENSION_IPI) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_RFENCE_SIG) ? (1<<SBI_EXTENSION_RFENCE) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_HSM_SIG) ? (1<<SBI_EXTENSION_HSM) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_SRST_SIG) ? (1<<SBI_EXTENSION_SRST) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_PMU_SIG) ? (1<<SBI_EXTENSION_PMU) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_DBCN_SIG) ? (1<<SBI_EXTENSION_DBCN) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_SUSP_SIG) ? (1<<SBI_EXTENSION_SUSP) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_CPPC_SIG) ? (1<<SBI_EXTENSION_CPPC) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_NACL_SIG) ? (1<<SBI_EXTENSION_NACL) : 0;
    sbi_ext |= sbi_probe_extension(SBI_EXT_STA_SIG) ? (1<<SBI_EXTENSION_STA) : 0;
}

void sbi_init(void) {
    ulong version = sbi_generic_call_2(SBI_GET_SBI_SPEC_VERSION).value;
    dprintf(INFO, "RISCV: SBI spec version %lu.%lu impl id %#lx version %#lx\n",
            (version >> 24) & 0x7f, version & ((1UL<<24)-1),
            sbi_generic_call_2(SBI_GET_SBI_IMPL_ID).value,
            sbi_generic_call_2(SBI_GET_SBI_IMPL_VERSION).value);

    // print the extensions detected
    if (LK_DEBUGLEVEL >= INFO) {
        dprintf(INFO, "RISCV: SBI extensions: ");
        if (sbi_ext_present(SBI_EXTENSION_TIMER)) dprintf(INFO, "TIMER ");
        if (sbi_ext_present(SBI_EXTENSION_IPI)) dprintf(INFO, "IPI ");
        if (sbi_ext_present(SBI_EXTENSION_RFENCE)) dprintf(INFO, "RFENCE ");
        if (sbi_ext_present(SBI_EXTENSION_HSM)) dprintf(INFO, "HSM ");
        if (sbi_ext_present(SBI_EXTENSION_SRST)) dprintf(INFO, "SRST ");
        if (sbi_ext_present(SBI_EXTENSION_PMU)) dprintf(INFO, "PMU ");
        if (sbi_ext_present(SBI_EXTENSION_DBCN)) dprintf(INFO, "DBCN ");
        if (sbi_ext_present(SBI_EXTENSION_SUSP)) dprintf(INFO, "SUSP ");
        if (sbi_ext_present(SBI_EXTENSION_CPPC)) dprintf(INFO, "CPPC ");
        if (sbi_ext_present(SBI_EXTENSION_NACL)) dprintf(INFO, "NACL ");
        if (sbi_ext_present(SBI_EXTENSION_STA)) dprintf(INFO, "STA ");
        dprintf(INFO, "\n");
    }
}

#endif

