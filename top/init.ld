SECTIONS {
    .init ALIGN(8) : {
        . = ALIGN(8);
        __lk_init = .;
        KEEP (*(.lk_init))
        __lk_init_end = .;
    }
}
INSERT AFTER .rodata;
