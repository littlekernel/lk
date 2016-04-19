LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
    dev/usb \
    dev/usb/class/cdcserial \


MODULE_SRCS += \
	$(LOCAL_DIR)/cdcserialtest.c \

include make/module.mk

