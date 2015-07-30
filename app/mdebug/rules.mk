LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/mdebug.c \
	$(LOCAL_DIR)/rswd.c \
	$(LOCAL_DIR)/swd-sgpio.c

include make/module.mk

