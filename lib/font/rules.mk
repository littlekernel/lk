LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/gfx

MODULE_SRCS += \
	$(LOCAL_DIR)/font.c

include make/module.mk
