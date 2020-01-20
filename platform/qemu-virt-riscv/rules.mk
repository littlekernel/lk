LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := riscv
SUBARCH ?= 32

MODULE_DEPS += lib/cbuf
MODULE_DEPS += lib/fdt

MODULE_SRCS += $(LOCAL_DIR)/platform.c
MODULE_SRCS += $(LOCAL_DIR)/plic.c
MODULE_SRCS += $(LOCAL_DIR)/uart.c

#ROMBASE ?= 0x20400000 # if running from rom, start here
ifeq ($(RISCV_MODE),supervisor)
MEMBASE ?= 0x080200000
else
MEMBASE ?= 0x080000000
endif
MEMSIZE ?= 0x00100000 # default to 1MB

# sifive_e or _u?
GLOBAL_DEFINES += PLATFORM_${VARIANT}=1

# set some global defines based on capability
GLOBAL_DEFINES += ARCH_RISCV_CLINT_BASE=0x02000000
GLOBAL_DEFINES += ARCH_RISCV_MTIME_RATE=10000000

# we're going to read the default memory map from a FDT
GLOBAL_DEFINES += NOVM_DEFAULT_ARENA=0

include make/module.mk
