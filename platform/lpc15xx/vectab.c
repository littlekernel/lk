/*
 * Copyright (c) 2013-2014 Travis Geiselbrecht
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

/* from cmsis.h */
#if 0
WDT_IRQn                      = 0,      /*!< Watchdog timer Interrupt                         */
WWDT_IRQn                     = WDT_IRQn,   /*!< Watchdog timer Interrupt alias for WDT_IRQn    */
BOD_IRQn                      = 1,      /*!< Brown Out Detect(BOD) Interrupt                  */
FMC_IRQn                      = 2,      /*!< FLASH Interrupt                                  */
FLASHEEPROM_IRQn              = 3,      /*!< EEPROM controller interrupt                      */
DMA_IRQn                      = 4,      /*!< DMA Interrupt                                    */
GINT0_IRQn                    = 5,      /*!< GPIO group 0 Interrupt                           */
GINT1_IRQn                    = 6,      /*!< GPIO group 1 Interrupt                           */
PIN_INT0_IRQn                 = 7,      /*!< Pin Interrupt 0                                  */
PIN_INT1_IRQn                 = 8,      /*!< Pin Interrupt 1                                  */
PIN_INT2_IRQn                 = 9,      /*!< Pin Interrupt 2                                  */
PIN_INT3_IRQn                 = 10,     /*!< Pin Interrupt 3                                  */
PIN_INT4_IRQn                 = 11,     /*!< Pin Interrupt 4                                  */
PIN_INT5_IRQn                 = 12,     /*!< Pin Interrupt 5                                  */
PIN_INT6_IRQn                 = 13,     /*!< Pin Interrupt 6                                  */
PIN_INT7_IRQn                 = 14,     /*!< Pin Interrupt 7                                  */
RITIMER_IRQn                  = 15,     /*!< RITIMER interrupt                                */
SCT0_IRQn                     = 16,     /*!< SCT0 interrupt                                   */
SCT_IRQn                      = SCT0_IRQn,  /*!< Optional alias for SCT0_IRQn                  */
SCT1_IRQn                     = 17,     /*!< SCT1 interrupt                                   */
SCT2_IRQn                     = 18,     /*!< SCT2 interrupt                                   */
SCT3_IRQn                     = 19,     /*!< SCT3 interrupt                                   */
MRT_IRQn                      = 20,     /*!< MRT interrupt                                    */
UART0_IRQn                    = 21,     /*!< UART0 Interrupt                                  */
UART1_IRQn                    = 22,     /*!< UART1 Interrupt                                  */
UART2_IRQn                    = 23,     /*!< UART2 Interrupt                                  */
I2C0_IRQn                     = 24,     /*!< I2C0 Interrupt                                   */
I2C_IRQn                      = I2C0_IRQn,  /*!< Optional alias for I2C0_IRQn                  */
SPI0_IRQn                     = 25,     /*!< SPI0 Interrupt                                   */
SPI1_IRQn                     = 26,     /*!< SPI1 Interrupt                                   */
CAN_IRQn                      = 27,     /*!< CAN Interrupt                                    */
USB0_IRQn                     = 28,     /*!< USB IRQ interrupt                                */
USB_IRQn                      = USB0_IRQn,  /*!< Optional alias for USB0_IRQn                  */
USB0_FIQ_IRQn                 = 29,     /*!< USB FIQ interrupt                                */
USB_FIQ_IRQn                  = USB0_FIQ_IRQn,  /*!< Optional alias for USB0_FIQ_IRQn         */
USB_WAKEUP_IRQn               = 30,     /*!< USB wake-up interrupt Interrupt                  */
ADC0_SEQA_IRQn                = 31,     /*!< ADC0_A sequencer Interrupt                       */
ADC0_A_IRQn                   = ADC0_SEQA_IRQn, /*!< Optional alias for ADC0_SEQA_IRQn        */
ADC_A_IRQn                    = ADC0_SEQA_IRQn, /*!< Optional alias for ADC0_SEQA_IRQn        */
ADC0_SEQB_IRQn                = 32,     /*!< ADC0_B sequencer Interrupt                       */
ADC0_B_IRQn                   = ADC0_SEQB_IRQn, /*!< Optional alias for ADC0_SEQB_IRQn        */
ADC_B_IRQn                    = ADC0_SEQB_IRQn, /*!< Optional alias for ADC0_SEQB_IRQn        */
ADC0_THCMP                    = 33,     /*!< ADC0 threshold compare interrupt                 */
ADC0_OVR                      = 34,     /*!< ADC0 overrun interrupt                           */
ADC1_SEQA_IRQn                = 35,     /*!< ADC1_A sequencer Interrupt                       */
ADC1_A_IRQn                   = ADC1_SEQA_IRQn, /*!< Optional alias for ADC1_SEQA_IRQn        */
ADC1_SEQB_IRQn                = 36,     /*!< ADC1_B sequencer Interrupt                       */
ADC1_B_IRQn                   = ADC1_SEQB_IRQn, /*!< Optional alias for ADC1_SEQB_IRQn        */
ADC1_THCMP                    = 37,     /*!< ADC1 threshold compare interrupt                 */
ADC1_OVR                      = 38,     /*!< ADC1 overrun interrupt                           */
DAC_IRQ                       = 39,     /*!< DAC interrupt                                    */
CMP0_IRQ                      = 40,     /*!< Analog comparator 0 interrupt                    */
CMP_IRQn                      = CMP0_IRQ,   /*!< Optional alias for CMP0_IRQ                    */
CMP1_IRQ                      = 41,     /*!< Analog comparator 1 interrupt                    */
CMP2_IRQ                      = 42,     /*!< Analog comparator 2 interrupt                    */
CMP3_IRQ                      = 43,     /*!< Analog comparator 3 interrupt                    */
QEI_IRQn                      = 44,     /*!< QEI interrupt                                    */
RTC_ALARM_IRQn                = 45,     /*!< RTC alarm interrupt                              */
RTC_WAKE_IRQn                 = 46,     /*!< RTC wake-up interrupt                            */
#endif

/* un-overridden irq handler */
void lpc_dummy_irq(void)
{
    arm_cm_irq_entry();

    panic("unhandled irq\n");
}

extern void lpc_uart_irq(void);

/* a list of default handlers that are simply aliases to the dummy handler */
#define DEFAULT_HANDLER(x) \
void lpc_##x##_irq(void) __WEAK_ALIAS("lpc_dummy_irq")

DEFAULT_HANDLER(WDT);
DEFAULT_HANDLER(BOD);
DEFAULT_HANDLER(FMC);
DEFAULT_HANDLER(FLASHEEPROM);
DEFAULT_HANDLER(DMA);
DEFAULT_HANDLER(GINT0);
DEFAULT_HANDLER(GINT1);
DEFAULT_HANDLER(PIN_INT0);
DEFAULT_HANDLER(PIN_INT1);
DEFAULT_HANDLER(PIN_INT2);
DEFAULT_HANDLER(PIN_INT3);
DEFAULT_HANDLER(PIN_INT4);
DEFAULT_HANDLER(PIN_INT5);
DEFAULT_HANDLER(PIN_INT6);
DEFAULT_HANDLER(PIN_INT7);
DEFAULT_HANDLER(RITIMER);
DEFAULT_HANDLER(SCT0);
DEFAULT_HANDLER(SCT1);
DEFAULT_HANDLER(SCT2);
DEFAULT_HANDLER(SCT3);
DEFAULT_HANDLER(MRT);
DEFAULT_HANDLER(UART0);
DEFAULT_HANDLER(UART1);
DEFAULT_HANDLER(UART2);
DEFAULT_HANDLER(I2C0);
DEFAULT_HANDLER(SPI0);
DEFAULT_HANDLER(SPI1);
DEFAULT_HANDLER(CAN);
DEFAULT_HANDLER(USB0);
DEFAULT_HANDLER(USB0_FIQ);
DEFAULT_HANDLER(USB_WAKEUP);
DEFAULT_HANDLER(ADC0_SEQA);
DEFAULT_HANDLER(ADC0_SEQB);
DEFAULT_HANDLER(ADC0_THCMP);
DEFAULT_HANDLER(ADC0_OVR);
DEFAULT_HANDLER(ADC1_SEQA);
DEFAULT_HANDLER(ADC1_SEQB);
DEFAULT_HANDLER(ADC1_THCMP);
DEFAULT_HANDLER(ADC1_OVR);
DEFAULT_HANDLER(DAC);
DEFAULT_HANDLER(CMP0);
DEFAULT_HANDLER(CMP1);
DEFAULT_HANDLER(CMP2);
DEFAULT_HANDLER(CMP3);
DEFAULT_HANDLER(QEI);
DEFAULT_HANDLER(RTC_ALARM);
DEFAULT_HANDLER(RTC_WAKE);

#define VECTAB_ENTRY(x) lpc_##x##_irq

const void *const __SECTION(".text.boot.vectab2") vectab2[] = {
    VECTAB_ENTRY(WDT),
    VECTAB_ENTRY(BOD),
    VECTAB_ENTRY(FMC),
    VECTAB_ENTRY(FLASHEEPROM),
    VECTAB_ENTRY(DMA),
    VECTAB_ENTRY(GINT0),
    VECTAB_ENTRY(GINT1),
    VECTAB_ENTRY(PIN_INT0),
    VECTAB_ENTRY(PIN_INT1),
    VECTAB_ENTRY(PIN_INT2),
    VECTAB_ENTRY(PIN_INT3),
    VECTAB_ENTRY(PIN_INT4),
    VECTAB_ENTRY(PIN_INT5),
    VECTAB_ENTRY(PIN_INT6),
    VECTAB_ENTRY(PIN_INT7),
    VECTAB_ENTRY(RITIMER),
    VECTAB_ENTRY(SCT0),
    VECTAB_ENTRY(SCT1),
    VECTAB_ENTRY(SCT2),
    VECTAB_ENTRY(SCT3),
    VECTAB_ENTRY(MRT),
    VECTAB_ENTRY(UART0),
    VECTAB_ENTRY(UART1),
    VECTAB_ENTRY(UART2),
    VECTAB_ENTRY(I2C0),
    VECTAB_ENTRY(SPI0),
    VECTAB_ENTRY(SPI1),
    VECTAB_ENTRY(CAN),
    VECTAB_ENTRY(USB0),
    VECTAB_ENTRY(USB0_FIQ),
    VECTAB_ENTRY(USB_WAKEUP),
    VECTAB_ENTRY(ADC0_SEQA),
    VECTAB_ENTRY(ADC0_SEQB),
    VECTAB_ENTRY(ADC0_THCMP),
    VECTAB_ENTRY(ADC0_OVR),
    VECTAB_ENTRY(ADC1_SEQA),
    VECTAB_ENTRY(ADC1_SEQB),
    VECTAB_ENTRY(ADC1_THCMP),
    VECTAB_ENTRY(ADC1_OVR),
    VECTAB_ENTRY(DAC),
    VECTAB_ENTRY(CMP0),
    VECTAB_ENTRY(CMP1),
    VECTAB_ENTRY(CMP2),
    VECTAB_ENTRY(CMP3),
    VECTAB_ENTRY(QEI),
    VECTAB_ENTRY(RTC_ALARM),
    VECTAB_ENTRY(RTC_WAKE),
};
