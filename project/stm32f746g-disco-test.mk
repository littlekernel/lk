include project/target/stm32f746g-disco.mk
include project/virtual/test.mk
include project/virtual/minip.mk
include project/virtual/mocom.mk
include project/virtual/fs.mk

MODULES += \
    app/loader

LK_HEAP_IMPLEMENTATION=cmpctmalloc

# Console serial port is on pins PA9(TX) and PB7(RX)
