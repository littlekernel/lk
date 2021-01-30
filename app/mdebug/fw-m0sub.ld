
/* RAM only binary layout */

SECTIONS {
	.text : {
		. = ALIGN(4);
		KEEP (*(.vectors))
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		. = ALIGN(4);
		__data_init__ = . ;
	} /* >RAM */
	.data : {
		. = ALIGN(4);
		__data_start__ = . ;
		*(.data)
		*(.data.*)
		. = ALIGN(4);
		__data_end__ = . ;
	} /* >RAM */
	.bss : {
		. = ALIGN(4);
		__bss_start__ = . ;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = . ;
	} /*>RAM */
}
