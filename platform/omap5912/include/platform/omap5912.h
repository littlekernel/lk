/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#ifndef __OMAP5912_H
#define __OMAP5912_H

/* memory map */
#define SDRAM_BASE	0x10000000

/* clocks */
#define DPLL_CTRL	(*(volatile unsigned short *)0xfffecf00)
#define ARM_CKCTL	(*(volatile unsigned int *)0xfffece00)
#define ARM_SYSST	(*(volatile unsigned int *)0xfffece18)

/* uart */
#define UART0_BASE 0xfffb0000
#define UART1_BASE 0xfffb0800
#define UART2_BASE 0xfffb9800

#define UART_RHR	0
#define UART_THR	0
#define UART_DLL	0
#define UART_IER	1
#define UART_DLH	1
#define UART_IIR	2
#define UART_FCR	2
#define UART_EFR	2
#define UART_LCR	3
#define UART_MCR	4
#define UART_LSR	5
#define UART_MSR	6
#define UART_TCR	6
#define UART_SPR	7
#define UART_TLR	7
#define UART_MDR1	8
#define UART_MDR2	9
#define UART_SFLSR	10
#define UART_RESUME	11
#define UART_TXFLL	10
#define UART_TXFLH	11
#define UART_SFREGL	12
#define UART_SFREGH	13
#define UART_RXFLL	12
#define UART_RXFLH	13
#define UART_BLR	14
#define UART_UASR	14
#define UART_ACREG	15
#define UART_SCR	16
#define UART_SSR	17
#define UART_EBLR	18
#define UART_MVR	19
#define UART_SYSC	20

/* timers */
#define MPU_TIMER0_BASE      0xfffec500
#define MPU_TIMER1_BASE      0xfffec600
#define MPU_TIMER2_BASE      0xfffec700
#define WATCHDOG_TIMER_BASE  0xfffec800
#define OS_TIMER_BASE        0xfffb9000
#define GP_TIMER1_BASE       0xfffb1400
#define GP_TIMER2_BASE       0xfffb1c00
#define GP_TIMER3_BASE       0xfffb2400
#define GP_TIMER4_BASE       0xfffb2c00
#define GP_TIMER5_BASE       0xfffb3400
#define GP_TIMER6_BASE       0xfffb3c00
#define GP_TIMER7_BASE       0xfffb4400
#define GP_TIMER8_BASE       0xfffb5c00

#define MPU_CNTL_TIMER1		(*(volatile unsigned int *)(MPU_TIMER1_BASE + 0x00))
#define MPU_LOAD_TIMER1		(*(volatile unsigned int *)(MPU_TIMER1_BASE + 0x04))
#define MPU_READ_TIMER1		(*(volatile unsigned int *)(MPU_TIMER1_BASE + 0x08))
#define MPU_CNTL_TIMER2		(*(volatile unsigned int *)(MPU_TIMER2_BASE + 0x00))
#define MPU_LOAD_TIMER2		(*(volatile unsigned int *)(MPU_TIMER2_BASE + 0x04))
#define MPU_READ_TIMER2		(*(volatile unsigned int *)(MPU_TIMER2_BASE + 0x08))
#define MPU_CNTL_TIMER3		(*(volatile unsigned int *)(MPU_TIMER3_BASE + 0x00))
#define MPU_LOAD_TIMER3		(*(volatile unsigned int *)(MPU_TIMER3_BASE + 0x04))
#define MPU_READ_TIMER3		(*(volatile unsigned int *)(MPU_TIMER3_BASE + 0x08))

#define OS_TIMER_TICK_VALUE_REG 	(*(volatile unsigned int *)(OS_TIMER_BASE + 0x00))
#define OS_TIMER_TICK_COUNTER_REG 	(*(volatile unsigned int *)(OS_TIMER_BASE + 0x04))
#define OS_TIMER_CTRL_REG		(*(volatile unsigned int *)(OS_TIMER_BASE + 0x08))


/* interrupt controller */
#define INT_VECTORS			(32 + 128)
#define INTCON0_BASE		0xfffecb00	
#define INTCON1_BASE		0xfffe0000
#define INTCON2_BASE		0xfffe0100
#define INTCON3_BASE		0xfffe0200
#define INTCON4_BASE		0xfffe0300

#define INTCON_ITR			0x00
#define INTCON_MIR			0x04
#define INTCON_SIR_IRQ		0x10
#define INTCON_SIR_FIQ		0x14
#define INTCON_CONTROL		0x18
#define INTCON_ILR_BASE		0x1c
#define INTCON_SISR			0x9c
#define INTCON_GMR			0xa0	/* only on first level controller */
#define INTCON_STATUS		0xa0	/* only on second level controllers */
#define INTCON_OCP_CFG		0xa4
#define INTCON_INTH_REV		0xa8

/* interrupts */
#define IRQ_TIMER3			16
#define IRQ_GPTIMER1		17
#define IRQ_GPTIMER2		18
#define IRQ_TIMER1			26
#define IRQ_WD_TIMER		27
#define IRQ_TIMER2			30
#define IRQ_OS_TIMER		(32 + 22)
#define IRQ_GPTIMER3		(32 + 34)
#define IRQ_GPTIMER4		(32 + 35)
#define IRQ_GPTIMER5		(32 + 36)
#define IRQ_GPTIMER6		(32 + 37)
#define IRQ_GPTIMER7		(32 + 38)
#define IRQ_GPTIMER8		(32 + 39)

#endif

