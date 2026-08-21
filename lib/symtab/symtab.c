/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/symtab.h>

#include <lk/console_cmd.h>
#include <stdio.h>

/* The table itself is generated from the symbols of a first link pass by
 * scripts/gen-symtab.py and linked into the image by symtab_buildrules.mk.
 * See lib/symtab/symtab_placeholder.c for the shape of an empty one.
 *
 * Addresses are held as offsets from lk_symtab_base, ascending, with one
 * extra entry at the end holding the end of the last function.
 */
extern const uintptr_t lk_symtab_base;
extern const uint32_t lk_symtab_count;
extern const uint32_t lk_symtab_addr[];
extern const uint32_t lk_symtab_nameoff[];
extern const char lk_symtab_names[];

const char *symtab_lookup(uintptr_t addr, uintptr_t *base_out) {
    const uint32_t count = lk_symtab_count;
    if (count == 0) {
        return NULL;
    }

    /* Reject anything outside the range the table describes before doing
     * arithmetic that would wrap on an address below the base.
     */
    if (addr < lk_symtab_base) {
        return NULL;
    }
    const uintptr_t offset = addr - lk_symtab_base;
    if (offset >= lk_symtab_addr[count]) {
        return NULL;
    }

    /* Find the last symbol starting at or below the address. */
    uint32_t lo = 0;
    uint32_t hi = count - 1;
    while (lo < hi) {
        const uint32_t mid = lo + (hi - lo + 1) / 2;
        if (lk_symtab_addr[mid] <= offset) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }

    if (base_out) {
        *base_out = lk_symtab_base + lk_symtab_addr[lo];
    }
    return lk_symtab_names + lk_symtab_nameoff[lo];
}

#if LK_DEBUGLEVEL > 1
static int cmd_sym(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
        printf("usage: %s <address>\n", argv[0].str);
        return -1;
    }

    const uintptr_t addr = (uintptr_t)argv[1].u;
    uintptr_t base;
    const char *name = symtab_lookup(addr, &base);
    if (!name) {
        printf("0x%lx: no symbol\n", (unsigned long)addr);
        return -1;
    }

    printf("0x%lx: %s+0x%lx\n", (unsigned long)addr, name, (unsigned long)(addr - base));
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("sym", "look up the symbol containing an address", &cmd_sym)
STATIC_COMMAND_END(symtab);
#endif
