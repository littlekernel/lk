/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>

/*
 * Per machine constants, taken from the qemu board models in hw/arm/mps2.c
 * (an385/an386/an500) and hw/arm/mps2-tz.c (an505/an547).
 *
 * The an505 and an547 are TrustZone machines: the cpu boots secure with the
 * SAU disabled, which makes the whole address space secure, so the plain
 * addresses below work as-is and there is nothing to set up. Those addresses
 * also have a secure alias at +0x10000000 if it ever needs to be explicit.
 */
#if MPS2_MACHINE_AN385 || MPS2_MACHINE_AN386 || MPS2_MACHINE_AN500
#define MPS2_UART0_BASE   0x40004000
#define MPS2_SYSCLK_HZ    25000000
#define MPS2_UART_PCLK_HZ 25000000
#elif MPS2_MACHINE_AN505
#define MPS2_UART0_BASE   0x40200000
#define MPS2_SYSCLK_HZ    20000000
#define MPS2_UART_PCLK_HZ 20000000
#elif MPS2_MACHINE_AN547
#define MPS2_UART0_BASE   0x49303000
#define MPS2_SYSCLK_HZ    32000000
/* the only machine where the peripheral clock is not the system clock */
#define MPS2_UART_PCLK_HZ 25000000
#else
#error "unknown mps2 machine"
#endif

__BEGIN_CDECLS

void mps2_debug_early_init(void);
void mps2_debug_init(void);

__END_CDECLS
