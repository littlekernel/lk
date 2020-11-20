SECTIONS {
  .text.bootloader_state : {
    *(.text.bootloader_state)
  }
}
INSERT AFTER .text;
