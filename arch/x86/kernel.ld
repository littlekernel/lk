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

ENTRY(_start)
SECTIONS
{
	.text 0x0200000 : {
		*(.text)
		*(SORT(.text$*))
		*(.gnu.linkonce.t.*)
		
		__ctor_list = .;
		*(SORT(.ctor*))
		__ctor_end = .;
		__dtor_list = .;
		*(SORT(.dtor*))
		__dtor_end = .;
		
		. = ALIGN(4096);
	}
	
	__data_start = .;
	.data		: { *(.data .data.* .gnu.linkonce.d.*) }
	.stab		: { *(.stab) }
	.stabst		: { *(.stabstr) }
	
	. = ALIGN(4096);
	__data_end = .;
	
	.rdata		: {
		*(.rdata)
		*(SORT(.rdata$*))
		*(.rodata*)
		*(.gnu.linkonce.r.*)
		__commands_start = .;
		KEEP (*(.commands))
		__commands_end = .;
		. = ALIGN(4);
		__apps_start = .;
		KEEP (*(.apps))
		__apps_end = .;
		
		. = ALIGN(4096);
	}
	
	__bss_start = .;
	.bss		: {
		*(.bss*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		
		. = ALIGN(4096);
	}
	__bss_end = .;
	
	/*. += 0x2000;
	_kstack = .;
	. += 0x1000;*/
	
	_end = .;
	
	/DISCARD/ : { *(.comment .note .eh_frame) }
}
