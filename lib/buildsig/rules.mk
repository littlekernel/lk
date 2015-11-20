LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/version

MODULE_SRCS := \
	$(LOCAL_DIR)/buildsig.c

include make/module.mk
