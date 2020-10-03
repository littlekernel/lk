LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/pll_read.c \
	$(LOCAL_DIR)/pll_control.c \

include make/module.mk
