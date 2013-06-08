LOCAL_DIR := $(GET_LOCAL_DIR)

#MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

PLATFORM := am335x

MODULES += \

MEMSIZE := 0x10000000	# 256MB

DEFINES += \

#include make/module.mk

