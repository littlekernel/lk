// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <lk/debug.h>
#include <lk/compiler.h>
#include <arch/arm/cm.h>

/* un-overridden irq handler */
void rp20xx_dummy_irq(void) {
    arm_cm_irq_entry();
    panic("unhandled irq\n");
}

/* a list of default handlers that are simply aliases to the dummy handler */
#define RP20XX_IRQ(name,num) \
void name##_IRQHandler(void) __WEAK_ALIAS("rp20xx_dummy_irq");
#include <platform/irqinfo.h>
#undef RP20XX_IRQ

const void* const __SECTION(".text.boot.vectab2") vectab2[26] = {
#define RP20XX_IRQ(name,num) [name##_IRQn] = name##_IRQHandler,
#include <platform/irqinfo.h>
#undef RP20XX_IRQ
};

