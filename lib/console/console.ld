SECTIONS {
    .commands ALIGN(8) : {
        __commands_start = .;
        KEEP (*(.commands))
        __commands_end = .;
    }
}
INSERT AFTER .data;
