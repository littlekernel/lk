/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/compiler.h>
#include <lk/main.h>
#include <stdint.h>

/* externals */
extern unsigned int __data_start_rom, __data_start, __data_end;
extern unsigned int __bss_start, __bss_end;

/* entry point of the binary */
void _start(void) {
    /* copy data from rom */
    if (&__data_start != &__data_start_rom) {
        unsigned int *src = &__data_start_rom;
        unsigned int *dest = &__data_start;

        while (dest != &__data_end)
            *dest++ = *src++;
    }

    /* zero out bss */
    unsigned int *bss = &__bss_start;
    while (bss != &__bss_end)
        *bss++ = 0;

    lk_main(0, 0, 0, 0);
}
