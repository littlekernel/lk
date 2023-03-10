/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define SIFIVE_IRQ_WATCHDOG 1
#define SIFIVE_IRQ_RTC   2
#define SIFIVE_IRQ_UART0 3
#define SIFIVE_IRQ_UART1 4
#define SIFIVE_IRQ_QSPI0 5
#define SIFIVE_IRQ_QSPI1 6
#define SIFIVE_IRQ_QSPI2 7
#define SIFIVE_IRQ_GPIO_BASE 8
#define SIFIVE_IRQ_GPIO(n) (SIFIVE_IRQ_GPIO_BASE+(n))
#define SIFIVE_IRQ_PWM_BASE 40

#define SIFIVE_NUM_IRQS 64

#define CLINT_BASE 0x02000000
#define PLIC_BASE  0x0c000000
#define AON_BASE   0x10000000
#define PRCI_BASE  0x10008000
#define OTP_BASE   0x10010000
#define GPIO_BASE  0x10012000
#define UART0_BASE 0x10013000
#define QSPI0_BASE 0x10014000
#define PWM0_BASE  0x10015000
#define UART1_BASE 0x10023000
#define QSPI1_BASE 0x10024000
#define PWM1_BASE  0x10025000
#define QSPI2_BASE 0x10034000
#define PWM2_BASE  0x10035000

#define GPIO_REG_VALUE      0
#define GPIO_REG_INPUT_EN   1
#define GPIO_REG_OUTPUT_EN  2
#define GPIO_REG_PORT       3
#define GPIO_REG_IOF_EN     14
#define GPIO_REG_IOF_SEL    15

#define GPIO_AF0 (1U << 16)
#define GPIO_AF1 (1U << 17)
