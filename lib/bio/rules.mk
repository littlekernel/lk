LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/bio.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/mem.c \
	$(LOCAL_DIR)/subdev.c

ifeq ($(call TOBOOL,$(WITH_TESTS)),true)
MODULE_DEPS += $(LOCAL_DIR)/test
endif

include make/module.mk
