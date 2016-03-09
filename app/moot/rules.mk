LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/fsboot.c \
	$(LOCAL_DIR)/moot.c \
	$(LOCAL_DIR)/stubs.c \
	$(LOCAL_DIR)/usbboot.c \


MODULE_DEPS += \
	lib/bootimage

include make/module.mk
