/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

static const char *
fpd_shift_prefix_32(int shift) {
    switch (shift) {
        case 32:
            return "";
        case 0:
            return "         ";
        case -32:
            return "                0.";
        case -64:
            return "                0.00000000 ";
        default:
            return "???";
    }
}

static const char *
fpd_shift_prefix_64(int shift) {
    switch (shift) {
        case 32:
            return "";
        case 0:
            return "         ";
        case -32:
            return "                  ";
        case -64:
            return "                         0.";
        default:
            return "???";
    }
}

static const char *
fpd_shift_suffix(int shift) {
    switch (shift) {
        case 32:
            return " 00000000                  ";
        case 0:
            return "                  ";
        case -32:
            return "         ";
        case -64:
            return "";
        default:
            return "???";
    }
}

static void
debug_mul_u32_u32(uint32_t a, uint32_t b, int a_shift, int b_shift, uint64_t ret) {
#if DEBUG_FIXED_POINT
    TRACEF("         %s%08x%s * %s%08x%s = %s%08x%s%08x%s\n",
           fpd_shift_prefix_32(a_shift), a, fpd_shift_suffix(a_shift),
           fpd_shift_prefix_32(b_shift), b, fpd_shift_suffix(b_shift),
           fpd_shift_prefix_64(a_shift + b_shift),
           (uint32_t)(ret >> 32),
           (a_shift + b_shift == -32) ? "." : " ",
           (uint32_t)ret,
           fpd_shift_suffix(a_shift + b_shift));
#endif
}

static void
debug_u64_mul_u32_fp32_64(uint32_t a, struct fp_32_64 b, uint64_t res_0, uint32_t res_l32_32, uint64_t ret) {
#if DEBUG_FIXED_POINT
    TRACEF("          %08x                   *          %08x.%08x %08x"
           " =          %08x %08x.%08x\n",
           a, b.l0, b.l32, b.l64,
           (uint32_t)(res_0 >> 32), (uint32_t)res_0, res_l32_32);
    TRACEF("                                   "
           "                                      "
           "~=          %08x %08x\n",
           (uint32_t)(ret >> 32), (uint32_t)ret);
#endif
}

static void
debug_u32_mul_u64_fp32_64(uint64_t a, struct fp_32_64 b, uint64_t res_l32, uint32_t ret) {
#if DEBUG_FIXED_POINT
    TRACEF("%08x %08x                   *          %08x.%08x %08x"
           " =                   %08x.%08x\n",
           (uint32_t)(a >> 32), (uint32_t)a, b.l0, b.l32, b.l64,
           (uint32_t)(res_l32 >> 32), (uint32_t)res_l32);
    TRACEF("                                   "
           "                                      "
           "~=                   %08x\n",
           ret);
#endif
}

static void
debug_u64_mul_u64_fp32_64(uint64_t a, struct fp_32_64 b, uint64_t res_0, uint32_t res_l32_32, uint64_t ret) {
#if DEBUG_FIXED_POINT
    TRACEF("%08x %08x                   *          %08x.%08x %08x"
           " =          %08x %08x.%08x\n",
           (uint32_t)(a >> 32), (uint32_t)a, b.l0, b.l32, b.l64,
           (uint32_t)(res_0 >> 32), (uint32_t)res_0, res_l32_32);
    TRACEF("                                   "
           "                                      "
           "~=          %08x %08x\n",
           (uint32_t)(ret >> 32), (uint32_t)ret);
#endif
}

