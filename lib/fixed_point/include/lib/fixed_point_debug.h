/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
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

#pragma once

static const char *
fpd_shift_prefix_32(int shift)
{
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
fpd_shift_prefix_64(int shift)
{
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
fpd_shift_suffix(int shift)
{
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
debug_mul_u32_u32(uint32_t a, uint32_t b, int a_shift, int b_shift, uint64_t ret)
{
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
debug_u64_mul_u32_fp32_64(uint32_t a, struct fp_32_64 b, uint64_t res_0, uint32_t res_l32_32, uint64_t ret)
{
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
debug_u32_mul_u64_fp32_64(uint64_t a, struct fp_32_64 b, uint64_t res_l32, uint32_t ret)
{
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
debug_u64_mul_u64_fp32_64(uint64_t a, struct fp_32_64 b, uint64_t res_0, uint32_t res_l32_32, uint64_t ret)
{
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

