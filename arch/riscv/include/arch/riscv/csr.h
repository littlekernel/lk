/*
 * Copyright (c) 2019 Elliot Berman
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <arch/riscv.h>

static inline ulong riscv_get_scratch(void) {
    return riscv_csr_read(RISCV_CSR_XSCRATCH);
}

#if RISCV_M_MODE
static inline long riscv_get_mvendorid(void) {
    return riscv_csr_read(RISCV_CSR_MVENDORID);
}

static inline long riscv_get_marchid(void) {
    return riscv_csr_read(RISCV_CSR_MARCHID);
}

static inline long riscv_get_mimpid(void) {
    return riscv_csr_read(RISCV_CSR_MIMPID);
}

#elif RISCV_S_MODE
#include <arch/riscv/sbi.h>

static inline long riscv_get_mvendorid(void) {
    struct sbiret ret = sbi_generic_call_2(SBI_GET_MVENDORID);
    return ret.value;
}

static inline long riscv_get_marchid(void) {
    struct sbiret ret = sbi_generic_call_2(SBI_GET_MARCHID);
    return ret.value;
}

static inline long riscv_get_mimpid(void) {
    struct sbiret ret = sbi_generic_call_2(SBI_GET_MIMPID);
    return ret.value;
}

#endif
