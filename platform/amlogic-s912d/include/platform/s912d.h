/*
 * Copyright (c) 2018 The Fuchsia Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */


#pragma once

#define SDRAM_BASE                      (0x0U)
#define AML_S912D_PERIPH_BASE_PHYS      (0xC0000000U)
#define AML_S912D_PERIPH_BASE_SIZE      (0x20000000U)
#define AML_S912D_PERIPH_BASE_VIRT      (0xFFFFFFFFC0000000ULL)

#define UART0_AO_BASE                   (AML_S912D_PERIPH_BASE_VIRT + 0x81004c0)
#define GIC_BASE                        (AML_S912D_PERIPH_BASE_VIRT + 0x4300000)

#define UART0_IRQ                       225
