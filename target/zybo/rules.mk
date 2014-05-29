LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

PLATFORM := zynq

# set the system base to sram
ZYNQ_USE_SRAM := 1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

GLOBAL_DEFINES += \
	EXTERNAL_CLOCK_FREQ=50000000 \
	WITH_STATIC_HEAP=1 \
	HEAP_START=0x00100000 \
	HEAP_LEN=0x1ff00000

MODULE_SRCS += \
	$(LOCAL_DIR)/target.c \
	$(LOCAL_DIR)/init.c

include make/module.mk
