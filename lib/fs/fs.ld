SECTIONS {
    .fs_impls : ALIGN(8) {
        __fs_impl_start = .;
        KEEP (*(.fs_impl))
        __fs_impl_end = .;
    }
}
INSERT AFTER .rodata;
