/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __ARM_M_ARCH_THREAD_H
#define __ARM_M_ARCH_THREAD_H

#include <stdbool.h>
#include <sys/types.h>

struct arch_thread {
    vaddr_t sp;
    bool was_preempted;

#if ARM_WITH_VFP
    /* has this thread ever used the floating point state? */
    bool fpused;

    /* s16-s31 saved here. s0-s15, fpscr saved on exception frame */
    float fpregs[16];
#endif
};

#endif

