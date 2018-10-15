LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

PLATFORM := sifive
SUBARCH ?= 32

MEMSIZE ?= 0x4000     # 16KB

#include make/module.mk

