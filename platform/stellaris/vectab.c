/*
 * Copyright (c) 2013 Travis Geiselbrecht
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

/* un-overridden irq handler */
void stellaris_dummy_irq(void)
{
    arm_cm_irq_entry();

    panic("unhandled irq\n");
}

extern void stellaris_uart_irq(void);

/* a list of default handlers that are simply aliases to the dummy handler */
#define DEFAULT_HANDLER(x) \
void stellaris_##x##_irq(void) __WEAK_ALIAS("stellaris_dummy_irq")

DEFAULT_HANDLER(gpio_porta);
DEFAULT_HANDLER(gpio_portb);
DEFAULT_HANDLER(gpio_portc);
DEFAULT_HANDLER(gpio_portd);
DEFAULT_HANDLER(gpio_porte);
DEFAULT_HANDLER(uart0);
DEFAULT_HANDLER(uart1);
DEFAULT_HANDLER(ssi0);
DEFAULT_HANDLER(i2c0);
DEFAULT_HANDLER(pwm_fault);
DEFAULT_HANDLER(pwm_gen0);
DEFAULT_HANDLER(pwm_gen1);
DEFAULT_HANDLER(pwm_gen2);
DEFAULT_HANDLER(quad_encoder0);
DEFAULT_HANDLER(adc_seq0);
DEFAULT_HANDLER(adc_seq1);
DEFAULT_HANDLER(adc_seq2);
DEFAULT_HANDLER(adc_seq3);
DEFAULT_HANDLER(watchdog_timer);
DEFAULT_HANDLER(timer0_subtimerA);
DEFAULT_HANDLER(timer0_subtimerB);
DEFAULT_HANDLER(timer1_subtimerA);
DEFAULT_HANDLER(timer1_subtimerB);
DEFAULT_HANDLER(timer2_subtimerA);
DEFAULT_HANDLER(timer2_subtimerB);
DEFAULT_HANDLER(analog_comp0);
DEFAULT_HANDLER(analog_comp1);
DEFAULT_HANDLER(analog_comp2);
DEFAULT_HANDLER(sys_control);
DEFAULT_HANDLER(flash_control);
DEFAULT_HANDLER(gpio_portf);
DEFAULT_HANDLER(gpio_portg);
DEFAULT_HANDLER(gpio_porth);
DEFAULT_HANDLER(uart2);
DEFAULT_HANDLER(ssi1);
DEFAULT_HANDLER(timer3_subtimerA);
DEFAULT_HANDLER(timer3_subtimerB);
DEFAULT_HANDLER(i2c1);
DEFAULT_HANDLER(quad_encoder1);
DEFAULT_HANDLER(can0);
DEFAULT_HANDLER(can1);
DEFAULT_HANDLER(can2);
DEFAULT_HANDLER(ethernet);
DEFAULT_HANDLER(hibernate);
DEFAULT_HANDLER(usb0);
DEFAULT_HANDLER(pwm_gen3);
DEFAULT_HANDLER(udma_software);
DEFAULT_HANDLER(udma_error);
DEFAULT_HANDLER(ad1_seq0);
DEFAULT_HANDLER(ad1_seq1);
DEFAULT_HANDLER(ad1_seq2);
DEFAULT_HANDLER(ad1_seq3);
DEFAULT_HANDLER(i2s0);
DEFAULT_HANDLER(ext_bus0);
DEFAULT_HANDLER(gpio_portj);
DEFAULT_HANDLER(gpio_portk);
DEFAULT_HANDLER(gpio_portl);
DEFAULT_HANDLER(ssi2);
DEFAULT_HANDLER(ssi3);
DEFAULT_HANDLER(uart3);
DEFAULT_HANDLER(uart4);
DEFAULT_HANDLER(uart5);
DEFAULT_HANDLER(uart6);
DEFAULT_HANDLER(uart7);
DEFAULT_HANDLER(i2c2);
DEFAULT_HANDLER(i2c3);
DEFAULT_HANDLER(timer4_subtimerA);
DEFAULT_HANDLER(timer4_subtimerB);
DEFAULT_HANDLER(timer5_subtimerA);
DEFAULT_HANDLER(timer5_subtimerB);
DEFAULT_HANDLER(wide_timer0_subtimerA);
DEFAULT_HANDLER(wide_timer0_subtimerB);
DEFAULT_HANDLER(wide_timer1_subtimerA);
DEFAULT_HANDLER(wide_timer1_subtimerB);
DEFAULT_HANDLER(wide_timer2_subtimerA);
DEFAULT_HANDLER(wide_timer2_subtimerB);
DEFAULT_HANDLER(wide_timer3_subtimerA);
DEFAULT_HANDLER(wide_timer3_subtimerB);
DEFAULT_HANDLER(wide_timer4_subtimerA);
DEFAULT_HANDLER(wide_timer4_subtimerB);
DEFAULT_HANDLER(wide_timer5_subtimerA);
DEFAULT_HANDLER(wide_timer6_subtimerB);
DEFAULT_HANDLER(fpu);
DEFAULT_HANDLER(peci0);
DEFAULT_HANDLER(lpc0);
DEFAULT_HANDLER(i2c4);
DEFAULT_HANDLER(i2c5);
DEFAULT_HANDLER(gpio_portm);
DEFAULT_HANDLER(gpio_portn);
DEFAULT_HANDLER(quad_encoder2);
DEFAULT_HANDLER(fan0);
DEFAULT_HANDLER(gpio_portp0);
DEFAULT_HANDLER(gpio_portp1);
DEFAULT_HANDLER(gpio_portp2);
DEFAULT_HANDLER(gpio_portp3);
DEFAULT_HANDLER(gpio_portp4);
DEFAULT_HANDLER(gpio_portp5);
DEFAULT_HANDLER(gpio_portp6);
DEFAULT_HANDLER(gpio_portp7);
DEFAULT_HANDLER(gpio_portq0);
DEFAULT_HANDLER(gpio_portq1);
DEFAULT_HANDLER(gpio_portq2);
DEFAULT_HANDLER(gpio_portq3);
DEFAULT_HANDLER(gpio_portq4);
DEFAULT_HANDLER(gpio_portq5);
DEFAULT_HANDLER(gpio_portq6);
DEFAULT_HANDLER(gpio_portq7);
DEFAULT_HANDLER(gpio_portr);
DEFAULT_HANDLER(gpio_ports);
DEFAULT_HANDLER(pwm1_gen0);
DEFAULT_HANDLER(pwm1_gen1);
DEFAULT_HANDLER(pwm1_gen2);
DEFAULT_HANDLER(pwm1_gen3);
DEFAULT_HANDLER(pwm1_fault);

#define VECTAB_ENTRY(x) stellaris_##x##_irq

const void *const __SECTION(".text.boot.vectab2") vectab2[] = {
    VECTAB_ENTRY(gpio_porta),               // GPIO Port A
    VECTAB_ENTRY(gpio_portb),               // GPIO Port B
    VECTAB_ENTRY(gpio_portc),               // GPIO Port C
    VECTAB_ENTRY(gpio_portd),               // GPIO Port D
    VECTAB_ENTRY(gpio_porte),               // GPIO Port E
    VECTAB_ENTRY(uart0),                    // UART0 Rx and Tx
    VECTAB_ENTRY(uart1),                    // UART1 Rx and Tx
    VECTAB_ENTRY(ssi0),                     // SSI0 Rx and Tx
    VECTAB_ENTRY(i2c0),                     // I2C0 Master and Slave
    VECTAB_ENTRY(pwm_fault),                // PWM Fault
    VECTAB_ENTRY(pwm_gen0),                 // PWM Generator 0
    VECTAB_ENTRY(pwm_gen1),                 // PWM Generator 1
    VECTAB_ENTRY(pwm_gen2),                 // PWM Generator 2
    VECTAB_ENTRY(quad_encoder0),            // Quadrature Encoder 0
    VECTAB_ENTRY(adc_seq0),                 // ADC Sequence 0
    VECTAB_ENTRY(adc_seq1),                 // ADC Sequence 1
    VECTAB_ENTRY(adc_seq2),                 // ADC Sequence 2
    VECTAB_ENTRY(adc_seq3),                 // ADC Sequence 3
    VECTAB_ENTRY(watchdog_timer),           // Watchdog timer
    VECTAB_ENTRY(timer0_subtimerA),         // Timer 0 subtimer A
    VECTAB_ENTRY(timer0_subtimerB),         // Timer 0 subtimer B
    VECTAB_ENTRY(timer1_subtimerA),         // Timer 1 subtimer A
    VECTAB_ENTRY(timer1_subtimerB),         // Timer 1 subtimer B
    VECTAB_ENTRY(timer2_subtimerA),         // Timer 2 subtimer A
    VECTAB_ENTRY(timer2_subtimerB),         // Timer 2 subtimer B
    VECTAB_ENTRY(analog_comp0),             // Analog Comparator 0
    VECTAB_ENTRY(analog_comp1),             // Analog Comparator 1
    VECTAB_ENTRY(analog_comp2),             // Analog Comparator 2
    VECTAB_ENTRY(sys_control),              // System Control (PLL, OSC, BO)
    VECTAB_ENTRY(flash_control),            // FLASH Control
    VECTAB_ENTRY(gpio_portf),               // GPIO Port F
    VECTAB_ENTRY(gpio_portg),               // GPIO Port G
    VECTAB_ENTRY(gpio_porth),               // GPIO Port H
    VECTAB_ENTRY(uart2),                    // UART2 Rx and Tx
    VECTAB_ENTRY(ssi1),                     // SSI1 Rx and Tx
    VECTAB_ENTRY(timer3_subtimerA),         // Timer 3 subtimer A
    VECTAB_ENTRY(timer3_subtimerB),         // Timer 3 subtimer B
    VECTAB_ENTRY(i2c1),                     // I2C1 Master and Slave
    VECTAB_ENTRY(quad_encoder1),            // Quadrature Encoder 1
    VECTAB_ENTRY(can0),                     // CAN0
    VECTAB_ENTRY(can1),                     // CAN1
    VECTAB_ENTRY(can2),                     // CAN2
    VECTAB_ENTRY(ethernet),                 // Ethernet
    VECTAB_ENTRY(hibernate),                // Hibernate
    VECTAB_ENTRY(usb0),                     // USB0
    VECTAB_ENTRY(pwm_gen3),                 // PWM Generator 3
    VECTAB_ENTRY(udma_software),            // uDMA Software Transfer
    VECTAB_ENTRY(udma_error),               // uDMA Error
    VECTAB_ENTRY(ad1_seq0),                 // ADC1 Sequence 0
    VECTAB_ENTRY(ad1_seq1),                 // ADC1 Sequence 1
    VECTAB_ENTRY(ad1_seq2),                 // ADC1 Sequence 2
    VECTAB_ENTRY(ad1_seq3),                 // ADC1 Sequence 3
    VECTAB_ENTRY(i2s0),                     // I2S0
    VECTAB_ENTRY(ext_bus0),                 // External Bus Interface 0
    VECTAB_ENTRY(gpio_portj),               // GPIO Port J
    VECTAB_ENTRY(gpio_portk),               // GPIO Port K
    VECTAB_ENTRY(gpio_portl),               // GPIO Port L
    VECTAB_ENTRY(ssi2),                     // SSI2 Rx and Tx
    VECTAB_ENTRY(ssi3),                     // SSI3 Rx and Tx
    VECTAB_ENTRY(uart3),                    // UART3 Rx and Tx
    VECTAB_ENTRY(uart4),                    // UART4 Rx and Tx
    VECTAB_ENTRY(uart5),                    // UART5 Rx and Tx
    VECTAB_ENTRY(uart6),                    // UART6 Rx and Tx
    VECTAB_ENTRY(uart7),                    // UART7 Rx and Tx
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(i2c2),                     // I2C2 Master and Slave
    VECTAB_ENTRY(i2c3),                     // I2C3 Master and Slave
    VECTAB_ENTRY(timer4_subtimerA),         // Timer 4 subtimer A
    VECTAB_ENTRY(timer4_subtimerB),         // Timer 4 subtimer B
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(timer5_subtimerA),         // Timer 5 subtimer A
    VECTAB_ENTRY(timer5_subtimerB),         // Timer 5 subtimer B
    VECTAB_ENTRY(wide_timer0_subtimerA),    // Wide Timer 0 subtimer A
    VECTAB_ENTRY(wide_timer0_subtimerB),    // Wide Timer 0 subtimer B
    VECTAB_ENTRY(wide_timer1_subtimerA),    // Wide Timer 1 subtimer A
    VECTAB_ENTRY(wide_timer1_subtimerB),    // Wide Timer 1 subtimer B
    VECTAB_ENTRY(wide_timer2_subtimerA),    // Wide Timer 2 subtimer A
    VECTAB_ENTRY(wide_timer2_subtimerB),    // Wide Timer 2 subtimer B
    VECTAB_ENTRY(wide_timer3_subtimerA),    // Wide Timer 3 subtimer A
    VECTAB_ENTRY(wide_timer3_subtimerB),    // Wide Timer 3 subtimer B
    VECTAB_ENTRY(wide_timer4_subtimerA),    // Wide Timer 4 subtimer A
    VECTAB_ENTRY(wide_timer4_subtimerB),    // Wide Timer 4 subtimer B
    VECTAB_ENTRY(wide_timer5_subtimerA),    // Wide Timer 5 subtimer A
    VECTAB_ENTRY(wide_timer6_subtimerB),    // Wide Timer 5 subtimer B
    VECTAB_ENTRY(fpu),                      // FPU
    VECTAB_ENTRY(peci0),                    // PECI 0
    VECTAB_ENTRY(lpc0),                     // LPC 0
    VECTAB_ENTRY(i2c4),                     // I2C4 Master and Slave
    VECTAB_ENTRY(i2c5),                     // I2C5 Master and Slave
    VECTAB_ENTRY(gpio_portm),               // GPIO Port M
    VECTAB_ENTRY(gpio_portn),               // GPIO Port N
    VECTAB_ENTRY(quad_encoder2),            // Quadrature Encoder 2
    VECTAB_ENTRY(fan0),                     // Fan 0
    VECTAB_ENTRY(dummy),                    // Reserved
    VECTAB_ENTRY(gpio_portp0),              // GPIO Port P (Summary or P0)
    VECTAB_ENTRY(gpio_portp1),              // GPIO Port P1
    VECTAB_ENTRY(gpio_portp2),              // GPIO Port P2
    VECTAB_ENTRY(gpio_portp3),              // GPIO Port P3
    VECTAB_ENTRY(gpio_portp4),              // GPIO Port P4
    VECTAB_ENTRY(gpio_portp5),              // GPIO Port P5
    VECTAB_ENTRY(gpio_portp6),              // GPIO Port P6
    VECTAB_ENTRY(gpio_portp7),              // GPIO Port P7
    VECTAB_ENTRY(gpio_portq0),              // GPIO Port Q (Summary or Q0)
    VECTAB_ENTRY(gpio_portq1),              // GPIO Port Q1
    VECTAB_ENTRY(gpio_portq2),              // GPIO Port Q2
    VECTAB_ENTRY(gpio_portq3),              // GPIO Port Q3
    VECTAB_ENTRY(gpio_portq4),              // GPIO Port Q4
    VECTAB_ENTRY(gpio_portq5),              // GPIO Port Q5
    VECTAB_ENTRY(gpio_portq6),              // GPIO Port Q6
    VECTAB_ENTRY(gpio_portq7),              // GPIO Port Q7
    VECTAB_ENTRY(gpio_portr),               // GPIO Port R
    VECTAB_ENTRY(gpio_ports),               // GPIO Port S
    VECTAB_ENTRY(pwm1_gen0),                // PWM 1 Generator 0
    VECTAB_ENTRY(pwm1_gen1),                // PWM 1 Generator 1
    VECTAB_ENTRY(pwm1_gen2),                // PWM 1 Generator 2
    VECTAB_ENTRY(pwm1_gen3),                // PWM 1 Generator 3
    VECTAB_ENTRY(pwm1_fault)                // PWM 1 Fault
};
