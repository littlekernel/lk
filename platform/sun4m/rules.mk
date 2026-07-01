LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := sparc

MODULE_SRCS += $(LOCAL_DIR)/debug.c
MODULE_SRCS += $(LOCAL_DIR)/intc.c
MODULE_SRCS += $(LOCAL_DIR)/platform.c
MODULE_SRCS += $(LOCAL_DIR)/timer.c

MEMBASE ?= 0x00000000
MEMSIZE ?= 0x04000000 # 64MB
KERNEL_LOAD_OFFSET := 0x4000 # seems to be where we're loaded

include make/module.mk
