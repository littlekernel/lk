SECTIONS {
    .apps : {
        __apps_start = .;
        KEEP (*(.apps))
        __apps_end = .;
    }
}
INSERT AFTER .rodata;
