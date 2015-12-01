LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := mips
MIPS_CPU := mips32

MODULE_DEPS += \
    lib/cbuf

MODULE_SRCS += \
	$(LOCAL_DIR)/intc.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/timer.c

MEMBASE ?= 0x0
MEMSIZE ?= 0x01000000 # 16MB

MODULE_DEPS += \

include make/module.mk
