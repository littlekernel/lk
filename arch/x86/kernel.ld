/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2013 Travis Geiselbrecht
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

ENTRY(_start)
SECTIONS
{
	.text 0x0200000 : {
		KEEP(*(.text.boot))
		*(.text* .sram.text)
		*(.gnu.linkonce.t.*)
	} =0x9090

	.rodata : ALIGN(4) {
		__rodata_start = .;
		*(.rodata*)
		*(.gnu.linkonce.r.*)
INCLUDE "arch/shared_rodata_sections.ld"
		. = ALIGN(4);
		__rodata_end = .;
	}

	.data : ALIGN(4096) {
		__data_start = .;
		*(.data .data.* .gnu.linkonce.d.*)
INCLUDE "arch/shared_data_sections.ld"
	}
	__ctor_list = .;
	.ctors : { KEEP(*(.ctors)) }
	__ctor_end = .;
	__dtor_list = .;
	.dtors : { KEEP(*(.dtors)) }
	__dtor_end = .;

	.stab   : { *(.stab) }
	.stabst : { *(.stabstr) }

	__data_end = .;

	.bss : ALIGN(4) {
		__bss_start = .;
		*(.bss*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	}

	_end = .;

	/DISCARD/ : { *(.comment .note .eh_frame) }
}
