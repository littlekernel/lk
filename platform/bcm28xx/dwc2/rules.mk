LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/dwc2.c \

# for 16bit chars in usb
GLOBAL_CFLAGS += -fshort-wchar

include make/module.mk
