LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/loader.c \

# disable a few warnings in gcc 11 that some of this module's code trips over
MODULE_COMPILEFLAGS += -Wno-array-bounds -Wno-stringop-overflow

MODULE_DEPS := \
    lib/cksum \
    lib/console \
    lib/tftp  \
    lib/elf

include make/module.mk
