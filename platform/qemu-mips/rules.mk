LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := mips
MIPS_CPU := m14k

MODULE_DEPS += \
    lib/cbuf

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/intc.c \
	$(LOCAL_DIR)/platform.c

MEMBASE ?= 0x80000000 # not exactly correct but gets us going for now
MEMSIZE ?= 0x01000000 # 16MB

MODULE_DEPS += \

include make/module.mk
