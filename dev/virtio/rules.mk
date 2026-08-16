LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/virtio-bus.cpp
MODULE_SRCS += $(LOCAL_DIR)/virtio-device.cpp
MODULE_SRCS += $(LOCAL_DIR)/virtio-mmio-bus.cpp
MODULE_SRCS += $(LOCAL_DIR)/virtio-pci-bus.cpp
MODULE_SRCS += $(LOCAL_DIR)/virtio.cpp

MODULE_DEPS += lib/libcpp

MODULE_WEAK_DEPS += \
	dev/bus/pci \
	dev/virtio/9p \
	dev/virtio/block \
	dev/virtio/gpu \
	dev/virtio/net \
	dev/virtio/rng

include make/module.mk
