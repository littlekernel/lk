LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

PLATFORM := or1ksim

MEMBASE ?= 0x00000000
MEMSIZE ?= 0x02000000

#include make/module.mk
