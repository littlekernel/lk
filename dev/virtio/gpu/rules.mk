LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/virtio-gpu.c \

MODULE_DEPS += \
	dev/virtio \
	lib/gfx

include make/module.mk
