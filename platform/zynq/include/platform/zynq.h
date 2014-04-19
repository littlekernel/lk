/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#pragma once

#include <reg.h>

/* hardware base addresses */
#define UART0_BASE (0xe0000000)
#define UART1_BASE (0xe0001000)
#define USB0_BASE  (0xe0002000)
#define USB1_BASE  (0xe0003000)
#define I2C0_BASE  (0xe0004000)
#define I2C1_BASE  (0xe0005000)
#define SPI0_BASE  (0xe0006000)
#define SPI1_BASE  (0xe0007000)
#define CAN0_BASE  (0xe0008000)
#define CAN1_BASE  (0xe0009000)
#define GPIO_BASE  (0xe000a000)
#define GEM0_BASE  (0xe000b000) // gigabit eth controller
#define GEM1_BASE  (0xe000c000) // ""
#define QSPI_BASE  (0xe000d000)
#define SMCC_BASE  (0xe000e000) // PL353 shared memory controller

#define SD0_BASE   (0xe0100000)
#define SD1_BASE   (0xe0101000)

#define SLCR_BASE  (0xf8000000)
#define TTC0_BASE  (0xf8001000)
#define TTC1_BASE  (0xf8002000)
#define DMAC0_NS_BASE (0xf8004000)
#define DMAC0_S_BASE (0xf8003000)
#define SWDT_BASE  (0xf8005000)

#define CPUPRIV_BASE      (0xf8f00000)
#define SCU_CONTROL_BASE  (CPUPRIV_BASE + 0x0000)
#define GIC_PROC_BASE     (CPUPRIV_BASE + 0x0100)
#define GLOBAL_TIMER_BASE (CPUPRIV_BASE + 0x0200)
#define PRIV_TIMER_BASE   (CPUPRIV_BASE + 0x0600)
#define GIC_DISTRIB_BASE  (CPUPRIV_BASE + 0x1000)
#define L2CACHE_BASE      (CPUPRIV_BASE + 0x2000)

#if 0
#define TIMER0 (0x10011000)
#define TIMER1 (0x10011020)
#define TIMER2 (0x10012000)
#define TIMER3 (0x10012020)
#define TIMER4 (0x10018000)
#define TIMER5 (0x10018020)
#define TIMER6 (0x10019000)
#define TIMER7 (0x10019020)
#endif

/* interrupts */
#define TTC0_A_INT  42
#define TTC0_B_INT  43
#define TTC0_C_INT  44
#define UART0_INT   59
#define UART1_INT   81
#define TTC1_A_INT  69
#define TTC2_B_INT  70
#define TTC3_C_INT  71

#define MAX_INT 96

