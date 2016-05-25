/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2012. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
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
