/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>

__BEGIN_CDECLS

void platform_init_console(void);

void set_visual_page(int page);
void set_active_page(int page);

int get_visual_page(void);
int get_active_page(void);

void place(int x,int y);
void cursor(int start, int end);

void _clear(char c, char attr, int x1, int y1, int x2, int y2);
void clear(void);

void _scroll(char attr, int x1, int y1, int x2, int y2);
void scroll(void);

void curr_save(void);
void curr_restore(void);

void cputc(char c);
void cputs(char *s);

void window(int x1, int y1, int x2, int y2);

void putc_xy(int x, int y, char attr, char c);
void puts_xy(int x, int y, char attr, char *s);

int printf_xy(int x, int y, char attr, char *fmt, ...) __PRINTFLIKE(4, 5);

#define CURSOR_BLOCK()  cursor(0, 15);
#define CURSOR_OFF()    cursor(16, 16);
#define CURSOR_STD()    cursor(14, 15);

/* text colors */
#define BLACK           0
#define BLUE            1
#define GREEN           2
#define CYAN            3
#define RED             4
#define MAGENTA         5
#define BROWN           6
#define LIGHTGRAY       7
#define DARKGRAY        8
#define LIGHTBLUE       9
#define LIGHTGREEN      10
#define LIGHTCYAN       11
#define LIGHTRED        12
#define LIGHTMAGENTA    13
#define YELLOW          14
#define WHITE           15

__END_CDECLS

