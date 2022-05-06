LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/bcache
MODULE_DEPS += lib/bio
MODULE_DEPS += lib/fs
MODULE_DEPS += lib/libcpp

ifeq ($(call TOBOOL,WITH_TESTS),true)
MODULE_DEPS += $(LOCAL_DIR)/test
endif

MODULE_SRCS += $(LOCAL_DIR)/dir.cpp
MODULE_SRCS += $(LOCAL_DIR)/fat.cpp
MODULE_SRCS += $(LOCAL_DIR)/file.cpp
MODULE_SRCS += $(LOCAL_DIR)/file_iterator.cpp
MODULE_SRCS += $(LOCAL_DIR)/fs.cpp

MODULE_COMPILEFLAGS += -Wmissing-declarations
MODULE_CPPFLAGS += -Wno-invalid-offsetof

include make/module.mk
