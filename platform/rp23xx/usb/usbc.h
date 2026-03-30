/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <dev/usbc.h>

__BEGIN_CDECLS

void rp23xx_usbc_early_init(void);
void rp23xx_usbc_init(void);

__END_CDECLS