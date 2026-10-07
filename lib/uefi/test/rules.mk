LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/uefi lib/bio lib/unittest
MODULE_SRCS += $(LOCAL_DIR)/loader_tests.cpp $(LOCAL_DIR)/io_buffer_tests.cpp
MODULE_DEFINES += LOCAL_DIR=\"$(LOCAL_DIR)\" GBL_EFI_DISABLE_CPP_ENUMS=1
MODULE_SRCDEPS += $(LOCAL_DIR)/../helloworld_aa64.efi

include make/module.mk
