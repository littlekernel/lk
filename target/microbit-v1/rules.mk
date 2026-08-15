LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# The v1 board carries the 16k/256k QFAA part. This must be set before the
# platform rules.mk is read, since that is where it selects MEMSIZE.
NRF51_CHIP := nrf51822-qfaa

PLATFORM := nrf51xxx

GLOBAL_DEFINES += \
	ENABLE_UART0=1 \

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

include make/module.mk
