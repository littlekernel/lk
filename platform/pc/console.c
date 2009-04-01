/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <arch/x86.h>
#include <platform/pc.h>
#include <platform/console.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* CGA values */
#define CURSOR_START		0x0A
#define CURSOR_END			0x0B
#define VIDEO_ADDRESS_MSB	0x0C
#define VIDEO_ADDRESS_LSB	0x0D
#define CURSOR_POS_MSB		0x0E
#define CURSOR_POS_LSB		0x0F

/* curr settings */
static unsigned char curr_x;
static unsigned char curr_y;
static unsigned char curr_start;
static unsigned char curr_end;
static unsigned char curr_attr;

/* video page buffer */
#define VPAGE_SIZE		2048
#define PAGE_MAX		8

static int active_page = 0;
static int visual_page = 0;

static int curs_x[PAGE_MAX];
static int curs_y[PAGE_MAX];

static struct {
	int x1, y1, x2, y2;
} view_window = {
	0, 0, 79, 24
};

void platform_init_console(void)
{
	curr_save();
	window(0, 0, 79, 24);
	clear();
	place(0, 0);
}

void set_visual_page(int page)
{
    unsigned short page_offset = page*VPAGE_SIZE;
    visual_page = page;
    
    outp(CGA_INDEX_REG, VIDEO_ADDRESS_LSB);
    outp(CGA_DATA_REG, page_offset & 0xFF);
    outp(CGA_INDEX_REG, VIDEO_ADDRESS_MSB);
    outp(CGA_DATA_REG, (page_offset >> 8) & 0xFF);
}

void set_active_page(int page)
{
    curs_x[active_page] = curr_x;
    curs_y[active_page] = curr_y;
    curr_x = curs_x[page];
    curr_y = curs_y[page];
    active_page = page;
}

int get_visual_page(void)
{
    return visual_page;
}

int get_active_page(void)
{
    return active_page;
}

void place(int x,int y)
{
    unsigned short cursor_word = x + y*80 + active_page*VPAGE_SIZE;
    
    /*
     * program CGA using index reg, then data reg
     */
    outp(CGA_INDEX_REG, CURSOR_POS_LSB);
    outp(CGA_DATA_REG, cursor_word & 0xFF);
    outp(CGA_INDEX_REG, CURSOR_POS_MSB);
    outp(CGA_DATA_REG, (cursor_word >> 8) & 0xFF);
    
    curr_x = x;
    curr_y = y;
}

void cursor(int start,int end)
{
    outp(CGA_INDEX_REG, CURSOR_START);
    outp(CGA_DATA_REG, start);
    outp(CGA_INDEX_REG, CURSOR_END);
    outp(CGA_DATA_REG, end);
}

void curr_save(void)
{
    /* grab some info from the bios data area (these should be defined in memmap.h */
    curr_attr = *((unsigned char *)0xB8000 + 159);
    curr_x = *((unsigned char *)0x00450);
    curr_y = *((unsigned char *)0x00451);
    curr_end = *((unsigned char *)0x00460);
    curr_start = *((unsigned char *)0x00461);
    active_page = visual_page = 0;
}

void curr_restore(void)
{
    *((unsigned char *)0x00450) = curr_x;
    *((unsigned char *)0x00451) = curr_y;
    
    place(curr_x, curr_y);
    cursor(curr_start, curr_end);
}

void window(int x1, int y1, int x2, int y2) {
	view_window.x1 = x1;
	view_window.y1 = y1;
	view_window.x2 = x2;
	view_window.y2 = y2;
	
	//place(x1, y1);
}

void _clear(char c,char attr,int x1,int y1,int x2,int y2)
{
	register int i,j;
	unsigned short w = attr;
	
	w <<= 8; w |= c;
	for (i = x1; i <= x2; i++) {
		for (j = y1; j <= y2; j++) { 
			*((unsigned short *)(0xB8000 + 2*i+160*j + 2*active_page*VPAGE_SIZE)) = w;
	    }
	}
	
	place(x1,y1);
	curr_y = y1;
	curr_x = x1;
}

void clear()
{
    _clear(' ', curr_attr, view_window.x1, view_window.y1, view_window.x2,
    	view_window.y2);
}

void _scroll(char attr, int x1, int y1, int x2, int y2)
{
    register int x,y;
    unsigned short xattr = attr << 8,w;
    unsigned char *v = (unsigned char *)(0xB8000 + active_page*(2*VPAGE_SIZE));
      
    for (y = y1+1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			w = *((unsigned short *) (v + 2*(y*80+x)));
			*((unsigned short *)(v + 2*((y-1)*80+x))) = w;
		}
	}
	
    for (x = x1; x <= x2; x++) {
   		*((unsigned short *)(v + 2*((y-1)*80+x))) = xattr;
   	}
}

void scroll(void)
{
    _scroll(curr_attr, view_window.x1, view_window.y1, view_window.x2,
    	view_window.y2);
}

void cputc(char c)
{
    static unsigned short scan_x, x, y;
    unsigned char *v = (unsigned char *)(0xB8000 + active_page*(2*VPAGE_SIZE));
    x = curr_x;
    y = curr_y;
    
    switch (c) {
	case '\t':
		x += 8;		
		if (x >= view_window.x2+1) {
			x = view_window.x1;
			if (y == view_window.y2) {
				scroll();
			} else {
				y++;
			}
		} else {
			scan_x = 0;
			
			while ((scan_x+8) < x) {
				scan_x += 8;
			}
			
			x = scan_x;
		}
		break;
		
	case '\n':
		x = view_window.x1;
		if (y == view_window.y2) {
			scroll();
		} else {
			y++;
		}
		break;
		
	case '\b':
		x--;
		*(v + 2*(x + y*80)) = ' ';
		break;
		
	default:
		*(v + 2*(x + y*80)) = c;
		x++;
    	
    	if (x >= view_window.x2+1) {
			x = view_window.x1;
			if (y == view_window.y2) {
				scroll();
			} else {
				y++;
			}
		}
    }
    
    place(x, y);
}

void cputs(char *s)
{
    char c;
    while (*s != '\0') {
		c = *s++;
		cputc(c);
    }
}

void puts_xy(int x,int y,char attr,char *s)
{
    unsigned char *v = (unsigned char *)(0xB8000 + (80*y+x)*2 + active_page*(2*VPAGE_SIZE));
    while (*s != 0) {
		*v = *s; s++; v++;
		*v = attr; v++;
    }
}

void putc_xy(int x, int y, char attr, char c)
{
    unsigned char *v = (unsigned char *)(0xB8000 + (80*y+x)*2 + active_page*(2*VPAGE_SIZE));
    *v = c; v++;
    *v = attr;
}

int printf_xy(int x, int y, char attr, char *fmt, ...)
{
	char cbuf[200];
	va_list parms;
	int result;
	
	va_start(parms, fmt);
	result = vsprintf(cbuf, fmt, parms);
	va_end(parms);
	
	puts_xy(x, y, attr, cbuf);
	
	return result;
}
