LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := riscv
SUBARCH ?= 32

MODULE_DEPS += lib/cbuf

MODULE_SRCS += $(LOCAL_DIR)/platform.c
MODULE_SRCS += $(LOCAL_DIR)/plic.c
MODULE_SRCS += $(LOCAL_DIR)/uart.c

ROMBASE ?= 0x20400000 # if running from rom, start here
MEMBASE ?= 0x80000000
MEMSIZE ?= 0x00100000 # default to 1MB

# set some global defines based on capability
GLOBAL_DEFINES += PLATFORM_HAS_DYNAMIC_TIMER=1
GLOBAL_DEFINES += ARCH_RISCV_CLINT_BASE=0x02000000
GLOBAL_DEFINES += ARCH_RISCV_MTIME_RATE=10000000 # 10Mhz

include make/module.mk
