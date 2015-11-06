SECTIONS {
    .devices : {
        __devices = .;
        KEEP (*(.devices))
        __devices_end = .;
    }
}
INSERT AFTER .data;
