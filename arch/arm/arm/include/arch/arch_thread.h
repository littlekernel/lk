/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __ARM_ARCH_THREAD_H
#define __ARM_ARCH_THREAD_H

#include <sys/types.h>

struct arch_thread {
    vaddr_t sp;

#if ARM_WITH_VFP
    /* has this thread ever used the floating point state? */
    bool fpused;

    uint32_t fpscr;
    uint32_t fpexc;
    double   fpregs[32];
#endif
};

#endif

