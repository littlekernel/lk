LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/ff.c \
	$(LOCAL_DIR)/option/ccsbcs.c \
	$(LOCAL_DIR)/os.c \
	$(LOCAL_DIR)/cmd.c \

include make/module.mk

