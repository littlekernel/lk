SECTIONS {
    .drivers : ALIGN(8) {
        __drivers = .;
        KEEP (*(.drivers))
        __drivers_end = .;
    }
}
INSERT AFTER .rodata;
