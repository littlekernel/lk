LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/mdebug.c \
	$(LOCAL_DIR)/rswd.c \
	$(LOCAL_DIR)/swo-uart1.c

#MODULE_SRCS +=  $(LOCAL_DIR)/swd-sgpio.c
MODULE_SRCS += $(LOCAL_DIR)/swd-m0sub.c

include make/module.mk

