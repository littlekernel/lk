SECTIONS {
    .apps ALIGN(8) : {
        __apps_start = .;
        KEEP (*(.apps))
        __apps_end = .;
    }
}
INSERT AFTER .data;
