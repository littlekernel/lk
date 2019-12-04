/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define SIFIVE_IRQ_UART0 3
#define SIFIVE_IRQ_UART1 4

#define SIFIVE_NUM_IRQS 127

#define CLINT_BASE 0x02000000
#define PLIC_BASE  0x0c000000
#define PRCI_BASE  0x10008000
#define GPIO_BASE  0x10012000
#define UART0_BASE 0x10013000

#define GPIO_REG_VALUE      0
#define GPIO_REG_INPUT_EN   1
#define GPIO_REG_OUTPUT_EN  2
#define GPIO_REG_PORT       3
#define GPIO_REG_IOF_EN     14
#define GPIO_REG_IOF_SEL    15

#define PLIC_HART_IDX(hart)    0
