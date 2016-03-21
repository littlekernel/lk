/*
 * Copyright (c) 2015 MediaTek Inc.
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
#ifndef __MT_GPT_H__
#define __MT_GPT_H__
#include <platform/mt_typedefs.h>
#include <sys/types.h>
#include <stdbool.h>

#define GPT_IRQEN_REG       ((volatile unsigned int*)(APXGPT_BASE))
#define GPT_IRQSTA_REG      ((volatile unsigned int*)(APXGPT_BASE+0x04))
#define GPT_IRQACK_REG      ((volatile unsigned int*)(APXGPT_BASE+0x08))

#define GPT4_CON_REG        ((volatile unsigned int*)(APXGPT_BASE+0x40))
#define GPT4_CLK_REG        ((volatile unsigned int*)(APXGPT_BASE+0x44))
#define GPT4_DAT_REG        ((volatile unsigned int*)(APXGPT_BASE+0x48))

#define GPT5_CON_REG        ((volatile unsigned int*)(APXGPT_BASE+0x50))
#define GPT5_CLK_REG        ((volatile unsigned int*)(APXGPT_BASE+0x54))
#define GPT5_COUNT_REG      ((volatile unsigned int*)(APXGPT_BASE+0x58))
#define GPT5_COMPARE_REG    ((volatile unsigned int*)(APXGPT_BASE+0x5C))

#define GPT_MODE4_ONE_SHOT (0x00 << 4)
#define GPT_MODE4_REPEAT   (0x01 << 4)
#define GPT_MODE4_KEEP_GO  (0x02 << 4)
#define GPT_MODE4_FREERUN  (0x03 << 4)

#define GPT_CLEAR       2

#define GPT_ENABLE      1
#define GPT_DISABLE     0

#define GPT_CLK_SYS     (0x0 << 4)
#define GPT_CLK_RTC     (0x1 << 4)

#define GPT_DIV_BY_1        0
#define GPT_DIV_BY_2        1

#define GPT4_EN                     0x0001
#define GPT4_FREERUN                0x0030
#define GPT4_SYS_CLK                0x0000

#define GPT4_1US_TICK       ((U32)13)           //    1000 / 76.92ns = 13.000
#define GPT4_1MS_TICK       ((U32)13000)        // 1000000 / 76.92ns = 13000.520
// 13MHz: 1us = 13.000 ticks
#define TIME_TO_TICK_US(us) ((us)*GPT4_1US_TICK + ((us)*0 + (1000-1))/1000)
// 13MHz: 1ms = 13000.520 ticks
#define TIME_TO_TICK_MS(ms) ((ms)*GPT4_1MS_TICK + ((ms)*520 + (1000-1))/1000)

extern void gpt_init(void);

#endif  /* !__MT_GPT_H__ */
