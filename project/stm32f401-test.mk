LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := stm32f401

MODULES += \
  app/shell \
  app/stringtests \
  app/tests \
  lib/cksum \
  lib/debugcommands \

WITH_CPP_SUPPORT=true

# Console serial port is on pins PA2(TX) and PA3(RX)
