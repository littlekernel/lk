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

#include "platform_p.h"

#define DEFAULT_PUTCHAR_ROUTINE putchar_mtfr
#define DEFAULT_GETCHAR_ROUTINE getchar_mtfr

// Console io via rom routine
unsigned int rom_putchar_addr = 0x20040068;
extern void rom_putchar(int c);
//unsigned int rom_getchar_addr = 0x20040068;
//extern void rom_getchar(int c);

// PR version
void putchar_mtfr(int c) {
    // wait until ready
    while ((mfpr(PR_TXCS) & 0x80) == 0)
        ;

    // output char
    mtpr((char)c, PR_TXDB);
}

int getchar_mtfr(void) {
    if (mfpr(PR_RXCS) & (1<<7)) {
        return mfpr(PR_RXDB) & 0xff;
    }
    return -1;
}

// select the above routine
void (*putchar_func)(int c) = &DEFAULT_PUTCHAR_ROUTINE;
int (*getchar_func)(void) = &DEFAULT_GETCHAR_ROUTINE;

void platform_dputc(char c) {
    if (c == '\n')
        putchar_func('\r');
    putchar_func(c);
}

int platform_dgetc(char *c, bool wait) {
    int ret = getchar_func();
    if (ret > 0) {
        *c = ret & 0xff;
        return 1;
    }
    return -1;
}

// crash time versions, for the moment use the above
void platform_pputc(char c) {
    platform_dputc(c);
}

int platform_pgetc(char *c, bool wait) {
    return platform_dgetc(c, wait);
}

void platform_early_console_init(void) {
    // TODO: decide what type of console to use based on vax machine

    // Only understand a few at the moment
    switch (vax_boardtype) {
        case 0x14000004: // Microvax 3100/40
            rom_putchar_addr = 0x20040068;
            putchar_func = &rom_putchar;
            //getchar_func = &rom_getchar;
            break;
        default:
            // default is to use the pr routines
            putchar_func = &putchar_mtfr;
            getchar_func = &getchar_mtfr;
    }
}


