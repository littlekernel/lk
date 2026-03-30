#define NO_PICO_PLATFORM 1
#include "boot/picobin.h"

.section .text.boot, "a"
.p2align 2

picobin_header:
// Minimal RP2350 IMAGE_DEF block loop for bootrom image scanning.
.word PICOBIN_BLOCK_MARKER_START
.byte PICOBIN_BLOCK_ITEM_1BS_IMAGE_TYPE
.byte 0x1
.hword PICOBIN_IMAGE_TYPE_IMAGE_TYPE_AS_BITS(EXE) | \
       PICOBIN_IMAGE_TYPE_EXE_SECURITY_AS_BITS(S) | \
       PICOBIN_IMAGE_TYPE_EXE_CPU_AS_BITS(ARM) | \
       PICOBIN_IMAGE_TYPE_EXE_CHIP_AS_BITS(RP2350)
.byte PICOBIN_BLOCK_ITEM_2BS_LAST
.hword 1
.byte 0
.word 0
.word PICOBIN_BLOCK_MARKER_END
