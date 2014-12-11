LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/partition

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/bio.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/mem.c \
	$(LOCAL_DIR)/subdev.c 

include make/module.mk
