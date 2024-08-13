LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/uefi.cpp \
	$(LOCAL_DIR)/text_protocol.cpp \
	$(LOCAL_DIR)/boot_service_provider.cpp \
	$(LOCAL_DIR)/runtime_service_provider.cpp \

include make/module.mk
