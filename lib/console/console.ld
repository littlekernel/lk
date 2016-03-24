SECTIONS {
    .commands : {
        __commands_start = .;
        KEEP (*(.commands))
        __commands_end = .;
    }
}
INSERT AFTER .data;
