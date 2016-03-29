/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2013 Travis Geiselbrecht
 * Copyright (c) 2015 Intel Corporation
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
    . = %KERNEL_BASE% + %KERNEL_LOAD_OFFSET%;

    .text : AT(%MEMBASE% + %KERNEL_LOAD_OFFSET%) {
        __code_start = .;
        KEEP(*(.text.boot))
        *(.text* .sram.text)
        *(.gnu.linkonce.t.*)
        __code_end = .;
    } =0x9090

    .rodata : ALIGN(4096) {
        __rodata_start = .;
        *(.rodata*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(8);
    }

    /*
     * extra linker scripts tend to insert sections just after .rodata,
     * so we want to make sure this symbol comes after anything inserted above,
     * but not aligned to the next section necessarily.
     */
    .dummy_post_rodata : {
        __rodata_end = .;
    }

    .data : ALIGN(4096) {
        __data_start = .;
        *(.data .data.* .gnu.linkonce.d.*)
    }

    .ctors : ALIGN(4) {
        __ctor_list = .;
        KEEP(*(.ctors .init_array))
        __ctor_end = .;
    }
    .dtors : ALIGN(4) {
        __dtor_list = .;
        KEEP(*(.dtors .fini_array))
        __dtor_end = .;
    }

    .stab   : { *(.stab) }
    .stabst : { *(.stabstr) }

    /*
     * extra linker scripts tend to insert sections just after .data,
     * so we want to make sure this symbol comes after anything inserted above,
     * but not aligned to the next section necessarily.
     */
    .dummy_post_data : {
        __data_end = .;
    }

    .bss : ALIGN(4096) {
        __bss_start = .;
        *(.bss*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
    }

    _end = .;

    /* put a symbol arbitrarily 4MB past the end of the kernel */
    /* used by the heap and other early boot time allocators */
    _end_of_ram = . + (4*1024*1024);

    /DISCARD/ : { *(.comment .note .eh_frame) }
}
