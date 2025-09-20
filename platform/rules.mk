LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# shared platform code
MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/power.c \
	$(LOCAL_DIR)/time.c \

MODULE_OPTIONS := extra_warnings

include make/module.mk


