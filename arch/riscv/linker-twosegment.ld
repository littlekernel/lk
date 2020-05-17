/*
 * LK linker script for single segment binaries.
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
    __rom_start = .;

    /* text/read-only data */
    /* set the load address to physical MEMBASE */
    .text : {
        KEEP(*(.text.boot))
        *(.text .text*)
        *(.gnu.linkonce.t.*)
    } :code

    .rodata : ALIGN(CONSTANT(MAXPAGESIZE)) {
        __rodata_start = .;
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        *(.srodata*)
    } :rodata

    /*
     * extra linker scripts tend to insert sections just after .rodata,
     * so we want to make sure this symbol comes after anything inserted above,
     * but not aligned to the next section necessarily.
     */
    .dummy_post_rodata : {
        __rodata_end = .;
        __rom_end = . ;
        __data_start_rom = .;
    }

    /* in two segment binaries, the data starts at the bottom of ram (MEMBASE)
     * bump us forward to the start of ram
     */
    . = %MEMBASE%;

    .data : AT ( ADDR (.dummy_post_rodata) + SIZEOF (.dummy_post_rodata) ) ALIGN(CONSTANT(MAXPAGESIZE)) {
        /* in one segment binaries, the rom data address is on top of the ram data address */
        __data_start = .;
        *(.data .data.* .gnu.linkonce.d.*)
        PROVIDE( __global_pointer$ = . + (4K / 2) );
        *(.sdata .sdata.*)
        __ctor_list = .;
        KEEP(*(.ctors .init_array))
        __ctor_end = .;
        __dtor_list = .;
        KEEP(*(.dtors .fini_array))
        __dtor_end = .;
        *(.got*)
        *(.dynamic)
    } :data

    /*
     * extra linker scripts tend to insert sections just after .data,
     * so we want to make sure this symbol comes after anything inserted above,
     * but not aligned to the next section necessarily.
     */
    .dummy_post_data : {
        . = ALIGN(%BITS% / 8);
        __data_end = .;
    }

    /* uninitialized data (in same segment as writable data) */
    .bss : ALIGN(%BITS% / 8) {
        __bss_start = .;
        /* regular bss */
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(.gnu.linkonce.b.*)

        . = ALIGN(%BITS% / 8);
        __bss_end = .;
    }

    _end = .;

    . = %MEMBASE% + %MEMSIZE%;
    _end_of_ram = .;

    /* Strip unnecessary stuff */
    /DISCARD/ : { *(.comment .note .eh_frame) }
}

