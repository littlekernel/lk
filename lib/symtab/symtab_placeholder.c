/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* An empty stand in for the generated symbol table.
 *
 * The table is generated from the symbols of a completed link, so the first
 * of the two link passes has to be satisfied with something: this. A count
 * of zero makes symtab_lookup() report that it knows no symbols, which is
 * also what keeps the intermediate image from claiming bogus names.
 */

#include <stdint.h>

const uintptr_t lk_symtab_base = 0;
const uint32_t lk_symtab_count = 0;
const uint32_t lk_symtab_addr[1] = { 0 };
const uint32_t lk_symtab_nameoff[1] = { 0 };
const char lk_symtab_names[1] = { 0 };
