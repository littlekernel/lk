/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/arm.h>

#define dsb()   DSB

#define mt_reg_sync_writel(v, a) \
        do {    \
            *(volatile unsigned int *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt_reg_sync_writew(v, a) \
        do {    \
            *(volatile unsigned short *)(a) = (v);    \
            dsb(); \
        } while (0)

#define mt_reg_sync_writeb(v, a) \
        do {    \
            *(volatile unsigned char *)(a) = (v);    \
            dsb(); \
        } while (0)

