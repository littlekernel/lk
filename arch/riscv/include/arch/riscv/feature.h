/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

__BEGIN_CDECLS

// Given a list of features in foo_bar_baz form, break out known features.
// Does not keep a pointer to the string.
void riscv_set_isa_string(const char *string);

enum riscv_feature {
    // Basic one character features
    RISCV_FEAT_I,
    RISCV_FEAT_M,
    RISCV_FEAT_A,
    RISCV_FEAT_F,
    RISCV_FEAT_D,
    RISCV_FEAT_Q,
    RISCV_FEAT_C,
    RISCV_FEAT_B,
    RISCV_FEAT_P,
    RISCV_FEAT_V,
    RISCV_FEAT_H,

    // multichar features
    RISCV_FEAT_ZBA,
    RISCV_FEAT_ZBB,
    RISCV_FEAT_ZBC,
    RISCV_FEAT_ZBS,
    RISCV_FEAT_ZICSR,
    RISCV_FEAT_ZIFENCEI,
    RISCV_FEAT_ZICBOM,
    RISCV_FEAT_ZICBOP,
    RISCV_FEAT_ZICBOZ,
    RISCV_FEAT_SSTC,
    RISCV_FEAT_SVADU,

    RISCV_FEAT_COUNT
};

extern uint32_t riscv_feature_bitmap[];

// Test if a particular feature is present
static inline bool riscv_feature_test(enum riscv_feature feature) {
    return riscv_feature_bitmap[feature / 32] & (1U << feature % 32);
}

const char *riscv_feature_to_string(enum riscv_feature feature);

void riscv_feature_early_init(void);
void riscv_feature_init(void);

__END_CDECLS
