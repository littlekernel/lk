LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/string_tests.c \
	$(LOCAL_DIR)/mymemcpy.S \
	$(LOCAL_DIR)/mymemset.S

include make/module.mk
