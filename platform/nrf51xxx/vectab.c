/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <debug.h>
#include <compiler.h>
#include <arch/arm/cm.h>
#include <lib/cbuf.h>
#include <platform/nrf51.h>
#include <target/debugconfig.h>

/* un-overridden irq handler */
void nrf51_dummy_irq(void)
{
    arm_cm_irq_entry();
    panic("unhandled irq\n");
}

/* a list of default handlers that are simply aliases to the dummy handler */
#define DEFAULT_HANDLER(x) \
void nrf51_##x(void) __WEAK_ALIAS("nrf51_dummy_irq");

DEFAULT_HANDLER(POWER_CLOCK_IRQ);
DEFAULT_HANDLER(RADIO_IRQ);
DEFAULT_HANDLER(UART0_IRQ);
DEFAULT_HANDLER(SPI0_TWI0_IRQ);
DEFAULT_HANDLER(SPI1_TWI1_IRQ);

DEFAULT_HANDLER(RESERVED_IRQ);
DEFAULT_HANDLER(GPIOTE_IRQ);
DEFAULT_HANDLER(ADC_IRQ);
DEFAULT_HANDLER(TIMER0_IRQ);
DEFAULT_HANDLER(TIMER1_IRQ);
DEFAULT_HANDLER(TIMER2_IRQ);

DEFAULT_HANDLER(RTC0_IRQ);
DEFAULT_HANDLER(TEMP_IRQ);
DEFAULT_HANDLER(RNG_IRQ);
DEFAULT_HANDLER(ECB_IRQ);
DEFAULT_HANDLER(CCM_AAR_IRQ);

DEFAULT_HANDLER(WDT_IRQ);
DEFAULT_HANDLER(RTC1_IRQ);
DEFAULT_HANDLER(QDEC_IRQ);
DEFAULT_HANDLER(LPCOMP_IRQ);
DEFAULT_HANDLER(SWI0_IRQ);

DEFAULT_HANDLER(SWI1_IRQ);
DEFAULT_HANDLER(SWI2_IRQ);
DEFAULT_HANDLER(SWI3_IRQ);
DEFAULT_HANDLER(SWI4_IRQ);
DEFAULT_HANDLER(SWI5_IRQ);


#define VECTAB_ENTRY(x) [x##n] = nrf51_##x

/* appended to the end of the main vector table */
const void *const __SECTION(".text.boot.vectab2") vectab2[] = {

    VECTAB_ENTRY(POWER_CLOCK_IRQ),
    VECTAB_ENTRY(RADIO_IRQ),
    VECTAB_ENTRY(UART0_IRQ),
    VECTAB_ENTRY(SPI0_TWI0_IRQ),
    VECTAB_ENTRY(SPI1_TWI1_IRQ),
    VECTAB_ENTRY(RESERVED_IRQ),
    VECTAB_ENTRY(GPIOTE_IRQ),
    VECTAB_ENTRY(ADC_IRQ),
    VECTAB_ENTRY(TIMER0_IRQ),
    VECTAB_ENTRY(TIMER1_IRQ),
    VECTAB_ENTRY(TIMER2_IRQ),

    VECTAB_ENTRY(RTC0_IRQ),
    VECTAB_ENTRY(TEMP_IRQ),
    VECTAB_ENTRY(RNG_IRQ),
    VECTAB_ENTRY(ECB_IRQ),
    VECTAB_ENTRY(CCM_AAR_IRQ),

    VECTAB_ENTRY(WDT_IRQ),
    VECTAB_ENTRY(RTC1_IRQ),
    VECTAB_ENTRY(QDEC_IRQ),
    VECTAB_ENTRY(LPCOMP_IRQ),
    VECTAB_ENTRY(SWI0_IRQ),

    VECTAB_ENTRY(SWI1_IRQ),
    VECTAB_ENTRY(SWI2_IRQ),
    VECTAB_ENTRY(SWI3_IRQ),
    VECTAB_ENTRY(SWI4_IRQ),
    VECTAB_ENTRY(SWI5_IRQ),
};

