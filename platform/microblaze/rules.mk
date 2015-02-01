LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := microblaze

MODULE_DEPS += \
    lib/cbuf

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/intc.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/uartlite.c

MEMBASE ?= 0x0
MEMSIZE ?= 0x20000	# 128KB

MODULE_DEPS += \

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE)

include make/module.mk
