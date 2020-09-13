/*
 * based heavily on https://github.com/ptesarik/vc4boot
 */

OUTPUT_FORMAT("elf32-vc4", "elf32-vc4", "elf32-vc4")
OUTPUT_ARCH(vc4)
ENTRY(_start)
GROUP(-lgcc)

MEMORY {
  /* in the C alias, direct uncached
   */
  ram (rwx) : ORIGIN = 0xc0000000 + 20M, LENGTH = 20M
}
SECTIONS {
  .text : {
    PROVIDE(_start = .);
    . = ALIGN(4);
    *(.text .stub .text.* .gnu.linkonce.t.*)
    *(.gnu.warning)
    KEEP (*(.init))
    KEEP (*(.fini))
    KEEP (*(.jcr))
    _etext = .;
  } >ram =0

  .rodata : {
    . = ALIGN(4);
    _frodata = .;
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.rodata1)
    _erodata = .;
  } >ram

  .init_array : {
    PROVIDE (__init_array_start = .);
    __ctor_list = .;
    *(.init_array)
    *(.init_array.*)
    *(.ctors)
    *(.ctors.*)
    __init_array_end = .;
    __ctor_end = .;
  } >ram

  .vectorTable : ALIGN(512) {
    *(.data.vectorTable)
  } >ram

  .rel : { *(.*.rel*) }

  .data : {
    . = ALIGN(4);
    _fdata = .;
    *(.data .data.* .gnu.linkonce.d.*)
    *(.data1)
    SORT(CONSTRUCTORS)
    *(.sdata .sdata.* .gnu.linkonce.s.*)
    _edata = .;
  } >ram

  .bss : {
    . = ALIGN(4);
    _fbss = .;
    *(.dynsbss)
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.scommon)
    *(.dynbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    _end = .;
    PROVIDE (end = .);
  } >ram

  /* First location in stack is highest address in RAM */
  PROVIDE(_fstack = ORIGIN(ram) + LENGTH(ram) - 4);
}
