LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/dev.c \
	$(LOCAL_DIR)/driver.c \

include make/module.mk
