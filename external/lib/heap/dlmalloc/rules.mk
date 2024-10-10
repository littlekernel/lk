LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEFINES=MSPACES=1

MODULE_SRCS += \
	$(LOCAL_DIR)/dlmalloc.c

include make/module.mk
