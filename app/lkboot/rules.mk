LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/console \
	lib/minip

MODULE_SRCS += \
	$(LOCAL_DIR)/lkboot.c \
	$(LOCAL_DIR)/commands.c

include make/module.mk
