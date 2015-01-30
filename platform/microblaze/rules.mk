LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := microblaze
#ARM_CPU := cortex-a9-neon

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c

MEMBASE := 0x0
MEMSIZE := 0x20000000	# 512MB

MODULE_DEPS += \

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE)

include make/module.mk
