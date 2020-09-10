MEMORY {
  ram (rwx) : ORIGIN = 0x80000000, LENGTH = 128K
}
GROUP(-lgcc)
SECTIONS {
  _start = .;
  .vectorTable : {
    . = 0x0;
    *(.data.vectorTable)
  }
  .text : {
    _text = .;
    *(.text.start)
    *(.text)
    *(.text.*)
    *(.gnu.warning)
    _etext = .;
  }

  .rodata : {
    *(.rodata)
    *(.rodata.*)
    _erodata = .;
  }

  .init_array : {
    . = ALIGN(16);
    PROVIDE(__ctor_list = .);
    KEEP(*(.init_array))
    KEEP(*(.init_array.*))
    KEEP(*(.ctors))
    KEEP(*(.ctors.*))
    __ctor_end = .;
  }

  .data : {
    *(.data)
    _edata = .;
  }

  .bss : {
    . = ALIGN(4);
    _fbss = .;
    *(.bss)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    _end = .;
  }
  _fstack = ORIGIN(ram) + LENGTH(ram) - 4;
}
/* ASSERT((_end - _start) <= LENGTH(ram), "bss wont fit within L2") */
