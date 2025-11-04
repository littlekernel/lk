LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/cbuf.c

ifeq ($(call TOBOOL,$(WITH_TESTS)),true)
MODULE_DEPS += $(LOCAL_DIR)/test
endif

include make/module.mk
