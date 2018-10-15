OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH(riscv)

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
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        *(.srodata*)
    }

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

    .data : AT ( ADDR (.dummy_post_rodata) + SIZEOF (.dummy_post_rodata) ) ALIGN(4) {
        /* in one segment binaries, the rom data address is on top of the ram data address */
        __data_start = .;
        *(.data .data.* .gnu.linkonce.d.*)
        *(.sdata .sdata.*)
        __ctor_list = .;
        KEEP(*(.ctors .init_array))
        __ctor_end = .;
        __dtor_list = .;
        KEEP(*(.dtors .fini_array))
        __dtor_end = .;
        *(.got*)
        *(.dynamic)
    }

    /*
     * extra linker scripts tend to insert sections just after .data,
     * so we want to make sure this symbol comes after anything inserted above,
     * but not aligned to the next section necessarily.
     */
    .dummy_post_data : {
        __data_end = .;
    }

    /* uninitialized data (in same segment as writable data) */
    .bss : ALIGN(4) {
        __bss_start = .;
        /* regular bss */
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(.gnu.linkonce.b.*)

        . = ALIGN(4);
        __bss_end = .;
    }

    _end = .;

    . = %MEMBASE% + %MEMSIZE%;
    _end_of_ram = .;

    /* Strip unnecessary stuff */
    /DISCARD/ : { *(.comment .note .eh_frame) }
}

