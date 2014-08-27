LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/minip \
	lib/sysparam \
	lib/ptable

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

include make/module.mk
