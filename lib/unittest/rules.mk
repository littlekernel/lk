LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_DEPS := \

MODULE_SRCS := \
	$(LOCAL_DIR)/unittest.c \
	$(LOCAL_DIR)/all_tests.c \

include make/module.mk
