LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/minip \
	lib/sysparam \
	lib/ptable

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

include make/module.mk
