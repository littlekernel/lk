LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

NRF51_CHIP := nrf51822-qfaa

PLATFORM := nrf51xxx

GLOBAL_DEFINES += \
	ENABLE_UART0=1 \
	

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

include make/module.mk

