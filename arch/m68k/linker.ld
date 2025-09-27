OUTPUT_FORMAT("elf32-m68k", "elf32-m68k", "elf32-m68k")
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
    . = %KERNEL_BASE% + %KERNEL_LOAD_OFFSET%;

    _start = .;

    /* text/read-only data */
    /* set the load address to physical MEMBASE */
    .text : AT(%MEMBASE% + %KERNEL_LOAD_OFFSET%) {
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

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
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

    . = ALIGN(32 / 8);
    __data_end = .;
    __bss_start = .;

    /* uninitialized data (in same segment as writable data) */
    .bss : {
        /* regular bss */
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
    }

    . = ALIGN(32 / 8);
    __bss_end = .;

    /* Align the end to ensure anything after the kernel ends up on its own pages */
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    _end = .;

    . = %KERNEL_BASE% + %MEMSIZE%;
    _end_of_ram = .;

    /* Strip unnecessary stuff */
    /DISCARD/ : { *(.comment .note .eh_frame) }
}

