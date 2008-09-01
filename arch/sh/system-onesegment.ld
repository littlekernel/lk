OUTPUT_FORMAT("elf32-shl", "elf32-shl", "elf32-shl")
OUTPUT_ARCH(sh)

ENTRY(_start)
SECTIONS
{
	. = %MEMBASE%;

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
	.init : { *(.init) } =0x9090
	.plt : { *(.plt) }

	/* text/read-only data */
	.text :	{ *(.text .text.* .glue_7* .gnu.linkonce.t.*) } =0x9090

	.rodata : { 
		*(.rodata .rodata.* .gnu.linkonce.r.*) 
		___commands_start = .;
		KEEP (*(.commands))
		___commands_end = .;
		. = ALIGN(4); 
		___rodata_end = . ;		
	}

	/* writable data  */
	___data_start_rom = .;	/* in one segment binaries, the rom data address is on top of the ram data address */
	___data_start = .;
	.data : { *(.data .data.* .gnu.linkonce.d.*) }

	___ctor_list = .;
	.ctors : { *(.ctors) }
	___ctor_end = .;
	___dtor_list = .;
	.dtors : { *(.dtors) }
	___dtor_end = .;
	.got : { *(.got.plt) *(.got) }
	.dynamic : { *(.dynamic) }

	___data_end = .;

	/* unintialized data (in same segment as writable data) */
	. = ALIGN(4);
	___bss_start = .;
	.bss : { *(.bss .bss.*) }

	. = ALIGN(4); 
	__end = .;

	. = %MEMBASE% + %MEMSIZE%;
	__end_of_ram = .;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame) }
}
