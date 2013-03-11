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

const void * const __SECTION(".text.boot.vectab2") vectab2[] = {
	stellaris_dummy_irq,                    // GPIO Port A
	stellaris_dummy_irq,                    // GPIO Port B
	stellaris_dummy_irq,                    // GPIO Port C
	stellaris_dummy_irq,                    // GPIO Port D
	stellaris_dummy_irq,                    // GPIO Port E
	stellaris_uart_irq,                     // UART0 Rx and Tx
	stellaris_dummy_irq,                    // UART1 Rx and Tx
	stellaris_dummy_irq,                    // SSI0 Rx and Tx
	stellaris_dummy_irq,                    // I2C0 Master and Slave
	stellaris_dummy_irq,                    // PWM Fault
	stellaris_dummy_irq,                    // PWM Generator 0
	stellaris_dummy_irq,                    // PWM Generator 1
	stellaris_dummy_irq,                    // PWM Generator 2
	stellaris_dummy_irq,                    // Quadrature Encoder 0
	stellaris_dummy_irq,                    // ADC Sequence 0
	stellaris_dummy_irq,                    // ADC Sequence 1
	stellaris_dummy_irq,                    // ADC Sequence 2
	stellaris_dummy_irq,                    // ADC Sequence 3
	stellaris_dummy_irq,                    // Watchdog timer
	stellaris_dummy_irq,                    // Timer 0 subtimer A
	stellaris_dummy_irq,                    // Timer 0 subtimer B
	stellaris_dummy_irq,                    // Timer 1 subtimer A
	stellaris_dummy_irq,                    // Timer 1 subtimer B
	stellaris_dummy_irq,                    // Timer 2 subtimer A
	stellaris_dummy_irq,                    // Timer 2 subtimer B
	stellaris_dummy_irq,                    // Analog Comparator 0
	stellaris_dummy_irq,                    // Analog Comparator 1
	stellaris_dummy_irq,                    // Analog Comparator 2
	stellaris_dummy_irq,                    // System Control (PLL, OSC, BO)
	stellaris_dummy_irq,                    // FLASH Control
	stellaris_dummy_irq,                    // GPIO Port F
	stellaris_dummy_irq,                    // GPIO Port G
	stellaris_dummy_irq,                    // GPIO Port H
	stellaris_dummy_irq,                    // UART2 Rx and Tx
	stellaris_dummy_irq,                    // SSI1 Rx and Tx
	stellaris_dummy_irq,                    // Timer 3 subtimer A
	stellaris_dummy_irq,                    // Timer 3 subtimer B
	stellaris_dummy_irq,                    // I2C1 Master and Slave
	stellaris_dummy_irq,                    // Quadrature Encoder 1
	stellaris_dummy_irq,                    // CAN0
	stellaris_dummy_irq,                    // CAN1
	stellaris_dummy_irq,                    // CAN2
	stellaris_dummy_irq,                    // Ethernet
	stellaris_dummy_irq,                    // Hibernate
	stellaris_dummy_irq,                    // USB0
	stellaris_dummy_irq,                    // PWM Generator 3
	stellaris_dummy_irq,                    // uDMA Software Transfer
	stellaris_dummy_irq,                    // uDMA Error
	stellaris_dummy_irq,                    // ADC1 Sequence 0
	stellaris_dummy_irq,                    // ADC1 Sequence 1
	stellaris_dummy_irq,                    // ADC1 Sequence 2
	stellaris_dummy_irq,                    // ADC1 Sequence 3
	stellaris_dummy_irq,                    // I2S0
	stellaris_dummy_irq,                    // External Bus Interface 0
	stellaris_dummy_irq,                    // GPIO Port J
	stellaris_dummy_irq,                    // GPIO Port K
	stellaris_dummy_irq,                    // GPIO Port L
	stellaris_dummy_irq,                    // SSI2 Rx and Tx
	stellaris_dummy_irq,                    // SSI3 Rx and Tx
	stellaris_dummy_irq,                    // UART3 Rx and Tx
	stellaris_dummy_irq,                    // UART4 Rx and Tx
	stellaris_dummy_irq,                    // UART5 Rx and Tx
	stellaris_dummy_irq,                    // UART6 Rx and Tx
	stellaris_dummy_irq,                    // UART7 Rx and Tx
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	stellaris_dummy_irq,                    // I2C2 Master and Slave
	stellaris_dummy_irq,                    // I2C3 Master and Slave
	stellaris_dummy_irq,                    // Timer 4 subtimer A
	stellaris_dummy_irq,                    // Timer 4 subtimer B
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	stellaris_dummy_irq,                    // Timer 5 subtimer A
	stellaris_dummy_irq,                    // Timer 5 subtimer B
	stellaris_dummy_irq,                    // Wide Timer 0 subtimer A
	stellaris_dummy_irq,                    // Wide Timer 0 subtimer B
	stellaris_dummy_irq,                    // Wide Timer 1 subtimer A
	stellaris_dummy_irq,                    // Wide Timer 1 subtimer B
	stellaris_dummy_irq,                    // Wide Timer 2 subtimer A
	stellaris_dummy_irq,                    // Wide Timer 2 subtimer B
	stellaris_dummy_irq,                    // Wide Timer 3 subtimer A
	stellaris_dummy_irq,                    // Wide Timer 3 subtimer B
	stellaris_dummy_irq,                    // Wide Timer 4 subtimer A
	stellaris_dummy_irq,                    // Wide Timer 4 subtimer B
	stellaris_dummy_irq,                    // Wide Timer 5 subtimer A
	stellaris_dummy_irq,                    // Wide Timer 5 subtimer B
	stellaris_dummy_irq,                    // FPU
	stellaris_dummy_irq,                    // PECI 0
	stellaris_dummy_irq,                    // LPC 0
	stellaris_dummy_irq,                    // I2C4 Master and Slave
	stellaris_dummy_irq,                    // I2C5 Master and Slave
	stellaris_dummy_irq,                    // GPIO Port M
	stellaris_dummy_irq,                    // GPIO Port N
	stellaris_dummy_irq,                    // Quadrature Encoder 2
	stellaris_dummy_irq,                    // Fan 0
	0,                                      // Reserved
	stellaris_dummy_irq,                    // GPIO Port P (Summary or P0)
	stellaris_dummy_irq,                    // GPIO Port P1
	stellaris_dummy_irq,                    // GPIO Port P2
	stellaris_dummy_irq,                    // GPIO Port P3
	stellaris_dummy_irq,                    // GPIO Port P4
	stellaris_dummy_irq,                    // GPIO Port P5
	stellaris_dummy_irq,                    // GPIO Port P6
	stellaris_dummy_irq,                    // GPIO Port P7
	stellaris_dummy_irq,                    // GPIO Port Q (Summary or Q0)
	stellaris_dummy_irq,                    // GPIO Port Q1
	stellaris_dummy_irq,                    // GPIO Port Q2
	stellaris_dummy_irq,                    // GPIO Port Q3
	stellaris_dummy_irq,                    // GPIO Port Q4
	stellaris_dummy_irq,                    // GPIO Port Q5
	stellaris_dummy_irq,                    // GPIO Port Q6
	stellaris_dummy_irq,                    // GPIO Port Q7
	stellaris_dummy_irq,                    // GPIO Port R
	stellaris_dummy_irq,                    // GPIO Port S
	stellaris_dummy_irq,                    // PWM 1 Generator 0
	stellaris_dummy_irq,                    // PWM 1 Generator 1
	stellaris_dummy_irq,                    // PWM 1 Generator 2
	stellaris_dummy_irq,                    // PWM 1 Generator 3
	stellaris_dummy_irq                     // PWM 1 Fault
};

