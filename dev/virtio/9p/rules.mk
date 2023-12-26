LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/client.c \
	$(LOCAL_DIR)/virtio-9p.c \
	$(LOCAL_DIR)/protocol.c

MODULE_DEPS += \
	dev/virtio

include make/module.mk
