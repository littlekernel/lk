LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/keys.o

ifeq ($(KEYS_USE_GPIO_KEYPAD),1)
OBJS += \
	$(LOCAL_DIR)/gpio_keypad.o
endif

