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
};

#endif

