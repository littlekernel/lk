SECTIONS {
  .text.bootloader_state : {
    *(.text.bootloader_state)
  }
  .text.firmware_rev : {
    *(.text.firmware_rev)
  }
}
INSERT AFTER .text;
