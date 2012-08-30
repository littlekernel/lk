LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/fs \
	lib/bcache \
	lib/bio

MODULE_SRCS += \
	$(LOCAL_DIR)/ext2.c \
	$(LOCAL_DIR)/dir.c \
	$(LOCAL_DIR)/io.c \
	$(LOCAL_DIR)/file.c

include make/module.mk
