OUTPUT_FORMAT("elf32-sparc", "elf32-sparc", "elf32-sparc")
OUTPUT_ARCH(sparc)

ENTRY(_start)
SECTIONS
{
    . = %KERNEL_BASE% + %KERNEL_LOAD_OFFSET%;

    /* text/read-only data */
    .text : {
        KEEP(*(.text.boot))
        *(.text* .gnu.linkonce.t.*)
    }

    .rodata : ALIGN(4) {
        __rodata_start = .;
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    .dummy_post_rodata : {
        __rodata_end = .;
    }

    .data : ALIGN(4) {
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
    }

    .dummy_post_data : {
        __data_end = .;
    }

    . = ALIGN(4);
    __bss_start = .;

    .bss : {
        *(.dynbss)
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
    }

    . = ALIGN(4);
    __bss_end = .;

    _end = .;

    . = %KERNEL_BASE% + %MEMSIZE%;
    _end_of_ram = .;

    /* Strip unnecessary stuff */
    /DISCARD/ : { *(.comment .note .eh_frame) }
}
