LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := sparc

MODULE_SRCS += $(LOCAL_DIR)/debug.cpp
MODULE_SRCS += $(LOCAL_DIR)/intc.cpp
MODULE_SRCS += $(LOCAL_DIR)/platform.cpp
MODULE_SRCS += $(LOCAL_DIR)/timer.cpp

MODULE_DEPS += lib/libcpp

MEMBASE ?= 0x00000000
MEMSIZE ?= 0x04000000 # 64MB
KERNEL_LOAD_OFFSET := 0x4000 # seems to be where we're loaded

include make/module.mk
