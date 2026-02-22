/*
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <hw/multiboot.h>

#if MULTIBOOT2_SUPPORT

__BEGIN_CDECLS

void platform_init_display(struct multiboot2_tag_framebuffer *framebuffer);

void place(unsigned int x, unsigned int y);

void clear_char(int x, int y);
void clear(void);

void window(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);

void scroll(void);

void dputc(char c);
void dputs(char *s);

void dputc_xy(unsigned int x, unsigned int y, char attr, char c);
void dputs_xy(unsigned int x, unsigned int y, char attr, char *s);

int display_printf_xy(unsigned int x, unsigned int y, char attr, char *fmt, ...) __PRINTFLIKE(4, 5);

__END_CDECLS

#endif // MULTIBOOT2_SUPPORT
