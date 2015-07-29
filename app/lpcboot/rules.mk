LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/lpcboot.c \
	$(LOCAL_DIR)/lpc43xx-spifi.c

include make/module.mk

