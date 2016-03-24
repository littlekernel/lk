SECTIONS {
    .fs_impls : {
        __fs_impl_start = .;
        KEEP (*(.fs_impl))
        __fs_impl_end = .;
    }
}
INSERT AFTER .rodata;
