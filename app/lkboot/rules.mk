LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/console \
	lib/minip \
	lib/sysparam

MODULE_SRCS += \
	$(LOCAL_DIR)/lkboot.c \
	$(LOCAL_DIR)/commands.c

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

include make/module.mk
