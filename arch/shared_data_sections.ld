/* should be included in all lk linker scripts in the .data segment */
. = ALIGN(4);
__commands_start = .;
KEEP (*(.commands))
__commands_end = .;
. = ALIGN(4);
__apps_start = .;
KEEP (*(.apps))
__apps_end = .;
. = ALIGN(4);
__devices = .;
KEEP(*(.devices))
__devices_end = .;

