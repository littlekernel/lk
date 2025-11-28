LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/bio \
	lib/fs \
	dev/virtio/9p

MODULE_SRCS += \
	$(LOCAL_DIR)/dir.c \
	$(LOCAL_DIR)/file.c \
	$(LOCAL_DIR)/v9fs.c

MODULE_OPTIONS := test

include make/module.mk
