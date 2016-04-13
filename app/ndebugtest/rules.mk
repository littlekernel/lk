LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/ndebugtest.c \

MODULE_DEPS += \
	lib/ndebug

include make/module.mk
