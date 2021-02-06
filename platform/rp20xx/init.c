// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <platform.h>
#include <arch/arm/cm.h>

extern void* vectab;

void platform_early_init(void) {
    // arch/arm/arm-m/arch.c does this but only for M3 and above...
    SCB->VTOR = (uint32_t) &vectab;

    arm_cm_systick_init(133000000);
}

void platform_init(void) {
}
