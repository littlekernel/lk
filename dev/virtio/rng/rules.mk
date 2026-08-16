LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/virtio-rng.cpp

MODULE_DEPS += dev/virtio

include make/module.mk
