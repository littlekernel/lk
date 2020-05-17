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
    . = %KERNEL_BASE% + %KERNEL_LOAD_OFFSET%;

    _start = .;

    /* text/read-only data */
    /* set the load address to physical MEMBASE */
    .text : AT(%MEMBASE% + %KERNEL_LOAD_OFFSET%) {
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
    }

    .data : ALIGN(CONSTANT(MAXPAGESIZE)) {
        __data_start_rom = .;
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

    /* Align the end to ensure anything after the kernel ends up on its own pages */
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    _end = .;

    . = %KERNEL_BASE% + %MEMSIZE%;
    _end_of_ram = .;

    /* Strip unnecessary stuff */
    /DISCARD/ : { *(.comment .note .eh_frame) }
}

