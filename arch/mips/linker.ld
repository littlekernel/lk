OUTPUT_FORMAT("elf32-bigmips", "elf32-bigmips", "elf32-littlemips")
OUTPUT_ARCH(mips)

ENTRY(_start)
SECTIONS
{
    . = %KERNEL_BASE% + %KERNEL_LOAD_OFFSET%;

    /* text/read-only data */
    .text : {
        KEEP(*(.text.vectab))
        KEEP(*(.text.boot))
        *(.text* .gnu.linkonce.t.*)
    }

    .interp : { *(.interp) }
    .hash : { *(.hash) }
    .dynsym : { *(.dynsym) }
    .dynstr : { *(.dynstr) }
    .rel.text : { *(.rel.text) *(.rel.gnu.linkonce.t*) }
    .rela.text : { *(.rela.text) *(.rela.gnu.linkonce.t*) }
    .rel.data : { *(.rel.data) *(.rel.gnu.linkonce.d*) }
    .rela.data : { *(.rela.data) *(.rela.gnu.linkonce.d*) }
    .rel.rodata : { *(.rel.rodata) *(.rel.gnu.linkonce.r*) }
    .rela.rodata : { *(.rela.rodata) *(.rela.gnu.linkonce.r*) }
    .rel.got : { *(.rel.got) }
    .rela.got : { *(.rela.got) }
    .rel.ctors : { *(.rel.ctors) }
    .rela.ctors : { *(.rela.ctors) }
    .rel.dtors : { *(.rel.dtors) }
    .rela.dtors : { *(.rela.dtors) }
    .rel.init : { *(.rel.init) }
    .rela.init : { *(.rela.init) }
    .rel.fini : { *(.rel.fini) }
    .rela.fini : { *(.rela.fini) }
    .rel.bss : { *(.rel.bss) }
    .rela.bss : { *(.rela.bss) }
    .rel.plt : { *(.rel.plt) }
    .rela.plt : { *(.rela.plt) }
    .init : { *(.init) }
    .plt : { *(.plt) }

    .rodata : ALIGN(4) {
        __rodata_start = .;
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    .sdata2 : ALIGN(4) {
        _SDATA2_START__ = .;
        *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
        _SDATA2_END__ = .;
    }

    .sbss2 : ALIGN(4) {
        /* read only small variables without initial value */
        _SBSS2_START__ = .;
        *(.sbss2*)
        _SBSS2_END__ = .;
    }

    /*
     * extra linker scripts tend to insert sections just after .rodata,
     * so we want to make sure this symbol comes after anything inserted above,
     * but not aligned to the next section necessarily.
     */
    .dummy_post_rodata : {
        __rodata_end = .;
    }

    .data : ALIGN(4) {
        /* writable data  */
        __data_start_rom = .;
        /* in one segment binaries, the rom data address is on top of the ram data address */
        __data_start = .;
        *(.data .data.* .gnu.linkonce.d.*)
        __ctor_list = .;
        KEEP(*(.ctors .init_array))
        __ctor_end = .;
        __dtor_list = .;
        KEEP(*(.dtors .fini_array))
        __dtor_end = .;
        *(.got*)
        *(.dynamic)

    }

    .sdata : {
        /* read-write small data with initial value */
        _SDATA_START__ = .;
        *(.sdata .sdata.* .gnu.linkonce.s.*)
        _SDATA_END__ = .;
    }

    /*
     * extra linker scripts tend to insert sections just after .data,
     * so we want to make sure this symbol comes after anything inserted above,
     * but not aligned to the next section necessarily.
     */
    .dummy_post_data : {
        __data_end = .;
    }

    . = ALIGN(4);
    __bss_start = .;

    .sbss : {
        /* read-write small variables without initial value */
        _sbss_start__ = .;
        *(.dynsbss)
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
        _sbss_end__ = .;
    }

    /* uninitialized data (in same segment as writable data) */
    .bss : {
        /* regular bss */
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

