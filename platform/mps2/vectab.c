/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/arm/cm.h>
#include <lk/compiler.h>
#include <lk/debug.h>

/* un-overridden irq handler */
static void mps2_dummy_irq(void) {
    arm_cm_irq_entry();

    panic("unhandled irq\n");
}

void mps2_uart0_rx_irq(void);

/*
 * Appended to the end of the main vector table, one entry per external
 * interrupt line. Everything the platform does not drive traps in the dummy
 * handler rather than running off into whatever follows the table.
 */
const void *const __SECTION(".text.boot.vectab2") vectab2[MPS2_NUM_IRQS] = {
#if MPS2_UART0_RX_IRQ > 0
    [0 ... MPS2_UART0_RX_IRQ - 1] = mps2_dummy_irq,
#endif
    [MPS2_UART0_RX_IRQ] = mps2_uart0_rx_irq,
    [MPS2_UART0_RX_IRQ + 1 ... MPS2_NUM_IRQS - 1] = mps2_dummy_irq,
};
