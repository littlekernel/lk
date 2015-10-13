include project/target/stm32f746g-disco.mk
include project/virtual/test.mk
include project/virtual/minip.mk

MODULES += \
    app/loader

# Console serial port is on pins PA9(TX) and PB7(RX)
