LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

STM32_CHIP := stm32f756

PLATFORM := stm32f7xx

GLOBAL_DEFINES += \
    ENABLE_UART3=1 \
    PLL_M_VALUE=8 \
    PLL_N_VALUE=336 \
    PLL_P_VALUE=2 \
\
    PKTBUF_POOL_SIZE=16 \
\


GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
    $(LOCAL_DIR)/init.c \
    $(LOCAL_DIR)/sensor_bus.c \
    $(LOCAL_DIR)/usb.c \

MODULE_DEPS += \
    dev/usb \
    lib/ndebug

include make/module.mk

