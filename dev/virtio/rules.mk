LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/virtio.c
MODULE_SRCS += $(LOCAL_DIR)/virtio_pci.c

include make/module.mk
