LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/cksum

MODULE_SRCS += \
	$(LOCAL_DIR)/flash_ptable.c

include make/module.mk
