/* should be included in all lk linker scripts in the .rodata segment */
. = ALIGN(8);
__lk_init = .;
KEEP(*(.lk_init))
__lk_init_end = .;
__drivers = .;
KEEP(*(.drivers))
__drivers_end = .;
