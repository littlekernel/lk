LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := riscv
SUBARCH ?= 32
VARIANT ?= nuclei

MODULE_SRCS += $(LOCAL_DIR)/platform.c
MODULE_SRCS += $(LOCAL_DIR)/uart.c
MODULE_SRCS += $(LOCAL_DIR)/vectab.c

ROMBASE ?= 0x80000000 # if running from rom, start here
MEMBASE ?= 0x90000000
MEMSIZE ?= 0x00010000 # default to 1MB

# uses a two segment layout, select the appropriate linker script
ARCH_RISCV_TWOSEGMENT := 1
# sets a few options in the riscv arch
ARCH_RISCV_EMBEDDED := 1

MODULE_DEPS += platform/hbird \
	lib/cbuf

include make/module.mk
