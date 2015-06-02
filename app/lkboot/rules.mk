LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/bio \
	lib/bootargs \
	lib/bootimage \
	lib/cbuf \
	lib/ptable \
	lib/sysparam

MODULE_SRCS += \
	$(LOCAL_DIR)/commands.c \
	$(LOCAL_DIR)/dcc.c \
	$(LOCAL_DIR)/inet.c \
	$(LOCAL_DIR)/lkboot.c \

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

include make/module.mk
