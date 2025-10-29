LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEFINES := \
	MSPACES=1 \
	GBL_EFI_DISABLE_CPP_ENUMS=1 \

MODULE_INCLUDES += \
	lib/watchdog/include \

MODULE_DEPS += \
	lib/libcpp \

MODULE_SRCS += \
	$(LOCAL_DIR)/uefi.cpp \
	$(LOCAL_DIR)/relocation.cpp \
	$(LOCAL_DIR)/text_protocol.cpp \
	$(LOCAL_DIR)/boot_service_provider.cpp \
	$(LOCAL_DIR)/memory_protocols.cpp \
	$(LOCAL_DIR)/blockio_protocols.cpp \
	$(LOCAL_DIR)/blockio2_protocols.cpp \
	$(LOCAL_DIR)/uefi_platform.cpp \
	$(LOCAL_DIR)/runtime_service_provider.cpp \
	$(LOCAL_DIR)/switch_stack.S \
	$(LOCAL_DIR)/configuration_table.cpp \
	$(LOCAL_DIR)/events.cpp \
	$(LOCAL_DIR)/io_stack.cpp \
	$(LOCAL_DIR)/debug_support.cpp \
	$(LOCAL_DIR)/charset.cpp \
	$(LOCAL_DIR)/variable_mem.cpp \


include make/module.mk
