LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

STM32_CHIP := stm32f746

PLATFORM := stm32f7xx

SDRAM_SIZE := 0x02000000
SDRAM_BASE := 0xc0000000
LCD_M_SIZE := 0x0012c000
HEAP_START := 0xc012c000
HEAP_SIZE  := 0x01ed4000

GLOBAL_DEFINES += \
    ENABLE_UART1=1 \
    ENABLE_SDRAM=1 \
    SDRAM_BASE=$(SDRAM_BASE) \
    SDRAM_SIZE=$(SDRAM_SIZE) \
\
    WITH_STATIC_HEAP=1 \
    HEAP_START=$(HEAP_START) \
    HEAP_LEN=$(HEAP_SIZE) \

# XXX todo, drive pll config from here
#HSE_VALUE=8000000 \
    PLL_M_VALUE=8 \
    PLL_N_VALUE=336 \
    PLL_P_VALUE=2

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
    $(LOCAL_DIR)/init.c \
    $(LOCAL_DIR)/lcd.c \
    $(LOCAL_DIR)/sdram.c

MODULE_DEPS += \
    lib/gfx \

#    lib/gfxconsole

include make/module.mk

