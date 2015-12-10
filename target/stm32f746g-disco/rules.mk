LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

STM32_CHIP := stm32f746

PLATFORM := stm32f7xx

SDRAM_SIZE := 0x00800000
SDRAM_BASE := 0xc0000000

GLOBAL_DEFINES += \
    ENABLE_UART1=1 \
    ENABLE_SDRAM=1 \
    USE_HSE_XTAL=1 \
    SDRAM_BASE=$(SDRAM_BASE) \
    SDRAM_SIZE=$(SDRAM_SIZE) \
    PLL_M_VALUE=8 \
    PLL_N_VALUE=336 \
    PLL_P_VALUE=2 \
\
    PKTBUF_POOL_SIZE=16


MODULE_SRCS += \
    $(LOCAL_DIR)/init.c \
    $(LOCAL_DIR)/lcd.c \
    $(LOCAL_DIR)/usb.c

MODULE_DEPS += \
    lib/gfx \
    dev/usb/class/bulktest

include make/module.mk

