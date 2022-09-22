/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/asm.h>

.section .text

/* used to call into PSCI firmware (Power State Coordination Firmware) */
FUNCTION(psci_call)
#if ARCH_ARM
    hvc     #0
    bx      lr
#else
    /* If booted at EL2 (bit 3 set of boot EL), we need to use SMC instead of HVC. */
    adrp    x9, arm64_boot_el
    ldr     x9, [x9, #:lo12:arm64_boot_el]
    tbnz    x9, #3, .Lsmc
    hvc     #0
    ret
.Lsmc:
    smc     #0
    ret
#endif

.ltorg


