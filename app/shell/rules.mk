LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/console

MODULE_SRCS += \
	$(LOCAL_DIR)/shell.c

include make/module.mk
