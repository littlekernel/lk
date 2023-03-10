LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := riscv
SUBARCH ?= 32
VARIANT ?= sifive_e

MODULE_DEPS += dev/gpio
MODULE_DEPS += dev/interrupt/riscv_plic
MODULE_DEPS += lib/cbuf

MODULE_SRCS += $(LOCAL_DIR)/platform.c
MODULE_SRCS += $(LOCAL_DIR)/uart.c
MODULE_SRCS += $(LOCAL_DIR)/gpio.c

ROMBASE ?= 0x20400000 # if running from rom, start here
MEMBASE ?= 0x80000000
MEMSIZE ?= 0x00100000 # default to 1MB

ifeq ($(VARIANT),sifive_e)
# uses a two segment layout, select the appropriate linker script
ARCH_RISCV_TWOSEGMENT := 1
# sets a few options in the riscv arch
ARCH_RISCV_EMBEDDED := 1

# disable WFI during idle. Have trouble breaking into a WFIed board
# with openocd.
GLOBAL_DEFINES += RISCV_DISABLE_WFI=1
endif

# sifive_e or _u?
GLOBAL_DEFINES += PLATFORM_${VARIANT}=1

include make/module.mk
