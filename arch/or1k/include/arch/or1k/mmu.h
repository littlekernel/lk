/*
 * Copyright (c) 2015 Stefan Kristiansson
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
#define SECTION_SIZE            (16U*1024U*1024U)

#define OR1K_MMU_PG_FLAGS_MASK  0x7ffU
#define OR1K_MMU_PG_PRESENT     0x400
#define OR1K_MMU_PG_L           0x200
#define OR1K_MMU_PG_X           0x100
#define OR1K_MMU_PG_W           0x080
#define OR1K_MMU_PG_U           0x040
#define OR1K_MMU_PG_D           0x020
#define OR1K_MMU_PG_A           0x010
#define OR1K_MMU_PG_WOM         0x008
#define OR1K_MMU_PG_WBC         0x004
#define OR1K_MMU_PG_CI          0x002
#define OR1K_MMU_PG_CC          0x001
