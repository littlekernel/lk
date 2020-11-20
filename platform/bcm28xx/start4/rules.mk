LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/start4.c

EXTRA_LINKER_SCRIPTS += $(LOCAL_DIR)/start4.ld

include make/module.mk
