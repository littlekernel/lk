LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/font

MODULE_SRCS += \
	$(LOCAL_DIR)/text.c

include make/module.mk
