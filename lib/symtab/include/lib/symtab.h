/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <lk/compiler.h>

__BEGIN_CDECLS

/* Look up the kernel text symbol containing addr.
 *
 * Returns the symbol name, or NULL if addr is outside the kernel's text or
 * the image was built without a symbol table. base_out, if given, receives
 * the address the symbol starts at, so the caller can report an offset.
 *
 * Safe to call from any context, including from a fault handler: it only
 * reads constant data and takes no locks.
 */
const char *symtab_lookup(uintptr_t addr, uintptr_t *base_out);

__END_CDECLS
