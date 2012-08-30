LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/fs.c \
	$(LOCAL_DIR)/debug.c

include make/module.mk
