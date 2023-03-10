/*
 * Copyright (c) 2019 Elliot Berman
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define SIFIVE_IRQ_UART0 4
#define SIFIVE_IRQ_UART1 5

#define SIFIVE_NUM_IRQS 53

#define CLINT_BASE 0x02000000
#define PLIC_BASE  0x0c000000
#define PRCI_BASE  0x10000000
#define UART0_BASE 0x10010000
#define UART1_BASE 0x10011000
#define PWM0_BASE  0x10020000
#define PWM1_BASE  0x10021000
#define GPIO_BASE  0x10060000
