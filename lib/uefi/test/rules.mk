LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEFINES := GBL_EFI_DISABLE_CPP_ENUMS=1

MODULE_SRCS += $(LOCAL_DIR)/hii_protocol_tests.cpp

MODULE_DEPS += \
	lib/uefi \
	lib/unittest \

include make/module.mk
