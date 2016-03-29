LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/mdebug.c \
	$(LOCAL_DIR)/rswd.c \
	$(LOCAL_DIR)/jtag.c \
	$(LOCAL_DIR)/swd-m0sub.c \
	$(LOCAL_DIR)/swo-uart1.c

include make/module.mk

