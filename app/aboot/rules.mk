LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

MODULE_SRCS += \
	$(LOCAL_DIR)/aboot.c \
	$(LOCAL_DIR)/fastboot.c

MODULE_DEPS += \
	lib/sysparam

include make/module.mk
