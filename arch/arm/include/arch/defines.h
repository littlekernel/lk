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
#ifndef __ARCH_CPU_H
#define __ARCH_CPU_H

/* arm specific stuff */
#define PAGE_SIZE 4096

#if ARM_CPU_ARM7
/* irrelevant, no consistent cache */
#define CACHE_LINE 32
#elif ARM_CPU_ARM926
#define CACHE_LINE 32
#elif ARM_CPU_ARM1136
#define CACHE_LINE 32
#elif ARM_CPU_CORTEX_A8
#define CACHE_LINE 64
#elif ARM_CPU_CORTEX_M3 || ARM_CPU_CORTEX_M4
#define CACHE_LINE 32 /* doesn't actually matter */
#else
#error unknown cpu
#endif

#endif

