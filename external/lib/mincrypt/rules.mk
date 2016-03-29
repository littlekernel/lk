LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/sha.c \
	$(LOCAL_DIR)/sha256.c

include make/module.mk
