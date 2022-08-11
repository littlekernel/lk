LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/app.c \
	$(LOCAL_DIR)/ksj.c

include make/module.mk
