LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/tests.c \
	$(LOCAL_DIR)/thread_tests.c \
	$(LOCAL_DIR)/printf_tests.c \
	$(LOCAL_DIR)/clock_tests.c \
	$(LOCAL_DIR)/benchmarks.c \
	$(LOCAL_DIR)/fibo.c

MODULE_COMPILEFLAGS += -Wno-format

include make/module.mk
