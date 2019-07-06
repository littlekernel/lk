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
    hvc     #0
#if ARCH_ARM
    bx      lr
#else
    ret
#endif

.ltorg

