LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/iskeleton.S \
	$(LOCAL_DIR)/os.S

include make/module.mk
