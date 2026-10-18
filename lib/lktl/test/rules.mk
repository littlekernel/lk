LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/auto_call_tests.cpp \
	$(LOCAL_DIR)/function_tests.cpp \
	$(LOCAL_DIR)/list_c_interop.c \
	$(LOCAL_DIR)/list_tests.cpp \
	$(LOCAL_DIR)/ref_ptr_tests.cpp \

MODULE_DEPS += \
	lib/libcpp \
	lib/lktl \
	lib/unittest

MODULE_OPTIONS := extra_warnings

include make/module.mk
