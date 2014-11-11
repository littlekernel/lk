/*
 * Copyright (c) 2008 Travis Geiselbrecht
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

#define SHIFT_4K        (12)
#define SHIFT_16K       (14)
#define SHIFT_64K       (16)

/* arm specific stuff */
#ifdef ARM64_LARGE_PAGESIZE_64K
#define PAGE_SIZE_SHIFT (SHIFT_64K)
#elif ARM64_LARGE_PAGESIZE_16K
#define PAGE_SIZE_SHIFT (SHIFT_16K)
#else
#define PAGE_SIZE_SHIFT (SHIFT_4K)
#endif
#define USER_PAGE_SIZE_SHIFT SHIFT_4K

#define PAGE_SIZE (1UL << PAGE_SIZE_SHIFT)
#define USER_PAGE_SIZE (1UL << USER_PAGE_SIZE_SHIFT)

#define CACHE_LINE 32

