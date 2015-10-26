SECTIONS {
    .drivers : {
        __drivers = .;
        KEEP (*(.drivers))
        __drivers_end = .;
    }
}
INSERT AFTER .rodata;
