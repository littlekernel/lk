LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

STM32_CHIP := stm32f746

PLATFORM := stm32f7xx

SDRAM_SIZE := 0x02000000
SDRAM_BASE := 0xc0000000
EXT_SRAM_BASE := 0x68000000
EXT_SRAM_SIZE := 0x00200000

GLOBAL_DEFINES += \
    ENABLE_UART1=1 \
    ENABLE_SDRAM=1 \
    SDRAM_BASE=$(SDRAM_BASE) \
    SDRAM_SIZE=$(SDRAM_SIZE) \
    EXT_SRAM_BASE=$(EXT_SRAM_BASE) \
    EXT_SRAM_SIZE=$(EXT_SRAM_SIZE) \
    ENABLE_EXT_SRAM=1 \
\
    PKTBUF_POOL_SIZE=16

# XXX todo, drive pll config from here
#HSE_VALUE=8000000 \
    PLL_M_VALUE=8 \
    PLL_N_VALUE=336 \
    PLL_P_VALUE=2

MODULE_SRCS += \
    $(LOCAL_DIR)/init.c \
    $(LOCAL_DIR)/lcd.c \
    $(LOCAL_DIR)/sram.c

MODULE_DEPS += \
    lib/gfx \

#    lib/gfxconsole

include make/module.mk

