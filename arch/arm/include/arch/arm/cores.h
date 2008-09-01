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
#ifndef __ARM_CORES_H
#define __ARM_CORES_H

/* 
 * make the gcc built in define a little easier to deal with 
 * to decide what core it is generating code for
 *
 * ARM_ARCH_LEVEL gets assigned a numeric value of the general family
 *
 * ARM_ARCH_* gets defined for each feature recursively
 */

#if defined(__ARM_ARCH_7M__)
#define ARM_ARCH_7M 1
#endif
#if defined(__ARM_ARCH_7R__)
#define ARM_ARCH_7R 1
#endif
#if defined(__ARM_ARCH_7A__) || defined(ARM_ARCH_7R)
#define ARM_ARCH_7A 1
#endif
#if defined(__ARM_ARCH_7__) || defined(ARM_ARCH_7A) || defined(ARM_ARCH_7M)
#define ARM_ARCH_7 1
#ifndef ARM_ARCH_LEVEL
#define ARM_ARCH_LEVEL 7
#endif
#endif

#if defined(__ARM_ARCH_6M__)
#define ARM_ARCH_6M 1
#endif
#if defined(__ARM_ARCH_6T2__) || defined(ARM_ARCH_7)
#define ARM_ARCH_6T2 1
#endif
#if defined(__ARM_ARCH_6ZK__)
#define ARM_ARCH_6ZK 1
#endif
#if defined(__ARM_ARCH_6Z__) || defined(ARM_ARCH_6ZK)
#define ARM_ARCH_6Z 1
#endif
#if defined(__ARM_ARCH_6K__) || defined(ARM_ARCH_6ZK) || defined(ARM_ARCH_7)
#define ARM_ARCH_6K 1
#endif
#if defined(__ARM_ARCH_6J__)
#define ARM_ARCH_6J 1
#endif
#if defined(__ARM_ARCH_6__) || defined(ARM_ARCH_6J) || defined(ARM_ARCH_6K) || defined(ARM_ARCH_6Z) || defined(ARM_ARCH_6T2) || defined(ARM_ARCH_6M)
#define ARM_ARCH_6 1
#ifndef ARM_ARCH_LEVEL
#define ARM_ARCH_LEVEL 6
#endif
#endif

#if defined(__ARM_ARCH_5TEJ__)
#define ARM_ARCH_5TEJ 1
#endif
#if defined(__ARM_ARCH_5TE__) || defined(ARM_ARCH_5TEJ) || defined(ARM_ARCH_6)
#define ARM_ARCH_5TE 1
#endif
#if defined(__ARM_ARCH_5E__) || defined(ARM_ARCH_5TE)
#define ARM_ARCH_5E 1
#endif
#if defined(__ARM_ARCH_5T__) || defined(ARM_ARCH_5TE)
#define ARM_ARCH_5T 1
#endif
#if defined(__ARM_ARCH_5__) || defined(ARM_ARCH_5E) || defined(ARM_ARCH_5T)
#define ARM_ARCH_5 1
#ifndef ARM_ARCH_LEVEL
#define ARM_ARCH_LEVEL 5
#endif
#endif

#if defined(__ARM_ARCH_4T__) || defined(ARM_ARCH_5T)
#define ARM_ARCH_4T 1
#endif
#if defined(__ARM_ARCH_4__) || defined(ARM_ARCH_4T) || defined(ARM_ARCH_5)
#define ARM_ARCH_4 1
#ifndef ARM_ARCH_LEVEL
#define ARM_ARCH_LEVEL 4
#endif
#endif

#if 0
/* test */
#if ARM_ARCH_LEVEL >= 7
#warning ARM_ARCH_LEVEL >= 7
#endif
#if ARM_ARCH_LEVEL >= 6
#warning ARM_ARCH_LEVEL >= 6
#endif
#if ARM_ARCH_LEVEL >= 5
#warning ARM_ARCH_LEVEL >= 5
#endif
#if ARM_ARCH_LEVEL >= 4
#warning ARM_ARCH_LEVEL >= 4
#endif
#endif

#endif

