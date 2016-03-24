LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/loader.c \


MODULE_DEPS := \
    lib/cksum \
    lib/tftp  \
    lib/elf

include make/module.mk
