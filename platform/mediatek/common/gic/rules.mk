LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/mt_gic_v3.c


include make/module.mk
