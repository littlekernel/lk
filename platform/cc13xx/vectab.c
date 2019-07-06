/*
 * Copyright (c) 2016 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/debug.h>
#include <lk/compiler.h>
#include <arch/arm/cm.h>

static void ti_cc_dummy_irq(void) {
    arm_cm_irq_entry();
    panic("unhandled irq");
}

#define DEFAULT_HANDLER(x) \
    void ti_cc_##x##_irq(void) __WEAK_ALIAS("ti_cc_dummy_irq")

#define DEFIRQ(n) DEFAULT_HANDLER(n);
#include <platform/defirq.h>
#undef DEFIRQ

#define VECTAB_ENTRY(x) ti_cc_##x##_irq

const void *const __SECTION(".text.boot.vectab2") vectab2[] = {
#define DEFIRQ(n) VECTAB_ENTRY(n),
#include <platform/defirq.h>
#undef DEFIRQ
};
