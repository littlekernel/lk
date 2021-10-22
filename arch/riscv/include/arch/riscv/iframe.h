/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define RISCV_IFRAME_EPC    (0 * __riscv_xlen / 8)
#define RISCV_IFRAME_STATUS (1 * __riscv_xlen / 8)
#define RISCV_IFRAME_RA     (2 * __riscv_xlen / 8)
#define RISCV_IFRAME_A_BASE (3 * __riscv_xlen / 8)
#define RISCV_IFRAME_A(n)   (RISCV_IFRAME_A_BASE + (n) * __riscv_xlen / 8)
#define RISCV_IFRAME_T_BASE (11 * __riscv_xlen / 8)
#define RISCV_IFRAME_T(n)   (RISCV_IFRAME_T_BASE + (n) * __riscv_xlen / 8)
#define RISCV_IFRAME_TP     (18 * __riscv_xlen / 8)
#define RISCV_IFRAME_GP     (19 * __riscv_xlen / 8)
#define RISCV_IFRAME_SP     (20 * __riscv_xlen / 8)

#define RISCV_IFRAME_LEN    (24 * __riscv_xlen / 8)

#ifndef ASSEMBLY

#include <lk/compiler.h>
#include <assert.h>

// keep in sync with asm.S
struct riscv_short_iframe {
    unsigned long  epc;
    unsigned long  status;
    unsigned long  ra;
    unsigned long  a0;
    unsigned long  a1;
    unsigned long  a2;
    unsigned long  a3;
    unsigned long  a4;
    unsigned long  a5;
    unsigned long  a6;
    unsigned long  a7;
    unsigned long  t0;
    unsigned long  t1;
    unsigned long  t2;
    unsigned long  t3;
    unsigned long  t4;
    unsigned long  t5;
    unsigned long  t6;
    // if we came from user space these are valid
    unsigned long  tp;
    unsigned long  gp;
    unsigned long  sp;
    unsigned long  pad[3]; // always maintain a padding of a multiple of 16 bytes
};

static_assert(sizeof(struct riscv_short_iframe) % 16 == 0, "");
static_assert(sizeof(struct riscv_short_iframe) == RISCV_IFRAME_LEN, "");

static_assert(offsetof(struct riscv_short_iframe, epc) == RISCV_IFRAME_EPC, "");
static_assert(offsetof(struct riscv_short_iframe, status) == RISCV_IFRAME_STATUS, "");
static_assert(offsetof(struct riscv_short_iframe, ra) == RISCV_IFRAME_RA, "");
static_assert(offsetof(struct riscv_short_iframe, a0) == RISCV_IFRAME_A_BASE, "");
static_assert(offsetof(struct riscv_short_iframe, t0) == RISCV_IFRAME_T_BASE, "");
static_assert(offsetof(struct riscv_short_iframe, gp) == RISCV_IFRAME_GP, "");
static_assert(offsetof(struct riscv_short_iframe, sp) == RISCV_IFRAME_SP, "");

#endif // __ASSEMBLY__

