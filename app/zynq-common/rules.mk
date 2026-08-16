LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/bootargs \
	lib/bootimage \
	lib/sysparam \
	lib/ptable

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

MODULE_DEPS += lib/watchdog

MODULE_WEAK_DEPS += lib/minip

include make/module.mk
