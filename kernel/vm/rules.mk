LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/bootalloc.c \
	$(LOCAL_DIR)/pmm.c \
	$(LOCAL_DIR)/vm.c \
	$(LOCAL_DIR)/vmm.c \

include make/module.mk
