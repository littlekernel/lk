LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/fs.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/shell.c

EXTRA_LINKER_SCRIPTS += $(LOCAL_DIR)/fs.ld

include make/module.mk
