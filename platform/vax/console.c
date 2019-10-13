/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <platform.h>
#include <platform/debug.h>

// todo: move to .S file
unsigned int rom_putchar_addr = 0x20040068;
extern int rom_putchar(int c);

// select the above routine
int (*putchar_func)(int c) = &rom_putchar;

void platform_dputc(char c) {
    if (c == '\n')
        putchar_func('\r');
    putchar_func(c);
}

int platform_dgetc(char *c, bool wait) {
    return -1;
}

// crash time versions, for the moment use the above
void platform_pputc(char c) {
    platform_dputc(c);
}

int platform_pgetc(char *c, bool wait) {
    return platform_dgetc(c, wait);
}



