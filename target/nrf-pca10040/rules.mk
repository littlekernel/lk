LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

NRF52_CHIP := nrf52832-qfaa

PLATFORM := nrf52xxx

GLOBAL_DEFINES += \
	ENABLE_UART0=1 \
	

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

include make/module.mk

