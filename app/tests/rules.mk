LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/tests.c \
	$(LOCAL_DIR)/thread_tests.c \
	$(LOCAL_DIR)/printf_tests.c \
	$(LOCAL_DIR)/clock_tests.c \
	$(LOCAL_DIR)/benchmarks.c

include make/module.mk
