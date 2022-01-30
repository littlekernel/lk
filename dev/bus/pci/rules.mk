LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/debug.cpp
MODULE_SRCS += $(LOCAL_DIR)/pci.cpp

MODULE_SRCS += $(LOCAL_DIR)/bus_mgr/bridge.cpp
MODULE_SRCS += $(LOCAL_DIR)/bus_mgr/bus.cpp
MODULE_SRCS += $(LOCAL_DIR)/bus_mgr/bus_mgr.cpp
MODULE_SRCS += $(LOCAL_DIR)/bus_mgr/device.cpp
MODULE_SRCS += $(LOCAL_DIR)/bus_mgr/resource.cpp

MODULE_SRCS += $(LOCAL_DIR)/backend/ecam.cpp
MODULE_SRCS += $(LOCAL_DIR)/backend/bios32.cpp
MODULE_SRCS += $(LOCAL_DIR)/backend/type1.cpp

MODULE_DEPS += lib/libcpp

MODULE_CPPFLAGS += -Wno-invalid-offsetof
MODULE_COMPILEFLAGS += -Wmissing-declarations

include make/module.mk
