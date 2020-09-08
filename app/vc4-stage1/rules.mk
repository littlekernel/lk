LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/partition \
	lib/fs \
	lib/fs/ext2 \
	lib/debugcommands \

MODULE_SRCS += \
	$(LOCAL_DIR)/stage1.c \

include make/module.mk

