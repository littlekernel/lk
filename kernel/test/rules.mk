LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/asid_tests.c \
	$(LOCAL_DIR)/clock_tests.c \
	$(LOCAL_DIR)/kill_signal_tests.c \
	$(LOCAL_DIR)/mp_tests.c \
	$(LOCAL_DIR)/port_tests.c \
	$(LOCAL_DIR)/thread_tests.c \

MODULE_DEPS += \
	kernel \
	lib/unittest

include make/module.mk
