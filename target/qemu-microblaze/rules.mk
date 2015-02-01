LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

PLATFORM := microblaze

MEMBASE ?= 0x90000000
MEMSIZE ?= 0x08000000

#include make/module.mk

