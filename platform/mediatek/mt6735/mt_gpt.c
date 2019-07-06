/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_gpt.h>

#define AP_PERI_GLOBALCON_PDN0 (PERICFG_BASE+0x10)

static void gpt_power_on(bool bPowerOn) {
    if (!bPowerOn) {
        DRV_SetReg32(AP_PERI_GLOBALCON_PDN0, 1<<13);
    } else {
        DRV_ClrReg32(AP_PERI_GLOBALCON_PDN0, 1<<13);
    }
}

static void gpt4_start(void) {
    DRV_WriteReg32(GPT4_CLK_REG, GPT4_SYS_CLK);
    DRV_WriteReg32(GPT4_CON_REG, GPT4_EN|GPT4_FREERUN);
}

static void gpt4_stop(void) {
    DRV_WriteReg32(GPT4_CON_REG, 0x0); // disable
    DRV_WriteReg32(GPT4_CON_REG, 0x2); // clear counter
}

static void gpt4_init(bool bStart) {
    gpt4_stop();

    if (bStart) {
        gpt4_start();
    }
}

void gpt_init(void) {
    gpt_power_on(TRUE);

    gpt4_init(TRUE);
}
