/* should be included in all lk linker scripts in the .rodata segment */
. = ALIGN(4);
__drivers = .;
KEEP(*(.drivers))
__drivers_end = .;
