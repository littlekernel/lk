OUTPUT_FORMAT("elf%BITS%-littleriscv")
/*
 * LK linker script for multi segment binaries.
 * In this case the read only portion of the binary lives in read only
 * memory, usually in ROM at a lower address.
 * Data and BSS live in a higher memory address and initial data is
 * copied from rom during initialization.
 */
PHDRS
{
    code PT_LOAD FLAGS(5);   /* PF_R|PF_X */
    rodata PT_LOAD FLAGS(4); /* PF_R */
    data PT_LOAD FLAGS(6);   /* PF_R|PF_W */
}

ENTRY(_start)
SECTIONS
{
    . = %ROMBASE%;

    _start = .;
    __rom_start = .;

    /* text/read-only data */
    /* set the load address to physical MEMBASE */
    .text : {
        KEEP(*(.text.boot))
        *(.text .text*)
        *(.gnu.linkonce.t.*)
    } :code

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        __rodata_start = .;
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    } :rodata

    /* trick to force any extra sections to be emitted here */
    . = .;

    __rodata_end = .;
    __rom_end = . ;
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    __data_start_rom = .;

    /* insert a dummy section that is used to anchor the following data segment */
    .dummy_post_rodata : { }

    /* in two segment binaries, the data starts at the bottom of ram (MEMBASE)
     * bump us forward to the start of ram
     */
    . = %MEMBASE%;

    .data : AT ( ADDR (.dummy_post_rodata) ) {
        __data_start = .;
        *(.data .data.* .gnu.linkonce.d.*)
        __ctor_list = .;
        KEEP(*(.ctors .init_array*))
        __ctor_end = .;
        __dtor_list = .;
        KEEP(*(.dtors .fini_array*))
        __dtor_end = .;
        *(.got*)
        *(.dynamic)
    } :data

    /* Try to put sdata and sbss near each other by putting sdata at the end of the data segment
     * and sbss at the start of the bss segment. This maximizes reach of things referenced off of
     * the global pointer. */
    .sdata : {
        __global_pointer$ = . + (4K / 2);
        /* Question: should we put srodata here on multi seg binaries? */
        *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata .srodata.*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
    }

    . = ALIGN(%BITS% / 8);
    __data_end = .;
    __bss_start = .;

    .sbss : {
        *(.dynsbss)
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
    }

    /* uninitialized data (in same segment as writable data) */
    .bss : {
        /* regular bss */
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
    }

    . = ALIGN(%BITS% / 8);
    __bss_end = .;

    /* Align the end to ensure anything after the kernel ends up on its own pages */
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    _end = .;

    . = %MEMBASE% + %MEMSIZE%;
    _end_of_ram = .;

    /* Strip unnecessary stuff */
    /DISCARD/ : { *(.comment .note .eh_frame) }
}

