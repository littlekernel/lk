SECTIONS {
    .devices ALIGN(8) : {
        __devices = .;
        KEEP (*(.devices))
        __devices_end = .;
    }
}
INSERT AFTER .data;
