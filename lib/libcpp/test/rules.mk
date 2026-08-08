LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/algorithm_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/array_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/atomic_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/cstddef_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/limits_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/memory_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/new_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/optional_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/span_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/string_view_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/type_traits_tests.cpp
MODULE_SRCS += $(LOCAL_DIR)/utility_tests.cpp

MODULE_DEPS += lib/libcpp
MODULE_DEPS += lib/unittest

include make/module.mk
