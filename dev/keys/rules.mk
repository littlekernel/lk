LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/keys.c

ifeq ($(KEYS_USE_GPIO_KEYPAD),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/gpio_keypad.c
endif

include make/module.mk

