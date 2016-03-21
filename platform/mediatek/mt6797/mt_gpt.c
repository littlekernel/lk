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
#include <sys/types.h>
#include <debug.h>
#include <err.h>
#include <reg.h>

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_gpt.h>

#define AP_PERI_GLOBALCON_PDN0 (PERICFG_BASE+0x10)

static void gpt_power_on(bool bPowerOn)
{
    if (!bPowerOn) {
        DRV_SetReg32(AP_PERI_GLOBALCON_PDN0, 1<<13);
    } else {
        DRV_ClrReg32(AP_PERI_GLOBALCON_PDN0, 1<<13);
    }
}

static void gpt4_start(void)
{
    DRV_WriteReg32(GPT4_CLK_REG, GPT4_SYS_CLK);
    DRV_WriteReg32(GPT4_CON_REG, GPT4_EN|GPT4_FREERUN);
}

static void gpt4_stop(void)
{
    DRV_WriteReg32(GPT4_CON_REG, 0x0); // disable
    DRV_WriteReg32(GPT4_CON_REG, 0x2); // clear counter
}

static void gpt4_init(bool bStart)
{
    gpt4_stop();

    if (bStart) {
        gpt4_start();
    }
}

void gpt_init(void)
{
    gpt_power_on(TRUE);

    gpt4_init(TRUE);
}

