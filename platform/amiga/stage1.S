/*
 * Copyright (c) 2025-2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

.text

lea     0(%pc),%a4
add.l   #0x200,%a4 /* skip bootblock */

jmp     (%a4)
