/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
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

/* echo | gcc -E -dM - to dump builtin defines */

#if defined(__ARM_ARCH_7EM__)
#define ARM_ARCH_7EM 1
#endif
#if defined(__ARM_ARCH_7M__) || defined(ARM_ARCH_7EM)
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

