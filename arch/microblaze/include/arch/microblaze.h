/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

static inline uint32_t mb_read_msr(void) {
    uint32_t temp;
    __asm__ volatile(
        "mfs    %0, rmsr;" : "=r" (temp));

    return temp;
}

static inline void mb_write_msr(uint32_t val) {
    __asm__ volatile(
        "mts    rmsr, %0" :: "r" (val));
}


