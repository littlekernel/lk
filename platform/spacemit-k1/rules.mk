LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := riscv
SUBARCH := 64
RISCV_MODE := supervisor
WITH_SMP ?= true
SMP_MAX_CPUS ?= 8
LK_HEAP_IMPLEMENTATION ?= dlmalloc
RISCV_FPU := true
RISCV_MMU := sv39
RISCV_EXTENSION_LIST ?= zba zbb zbc zbs

MODULE_DEPS += lib/cbuf
MODULE_DEPS += lib/fdt
MODULE_DEPS += lib/fdtwalk
MODULE_DEPS += dev/interrupt/riscv_plic

MODULE_SRCS += $(LOCAL_DIR)/platform.c
MODULE_SRCS += $(LOCAL_DIR)/uart.c

MEMBASE ?= 0
MEMSIZE ?= 0x80000000 # default to 2GB
ifeq ($(RISCV_MODE),supervisor)
# offset the kernel to account for OpenSBI using the bottom
KERNEL_LOAD_OFFSET ?= 0x10200000 # kernel load offset
endif

# we can revert to a poll based uart spin routine
GLOBAL_DEFINES += PLATFORM_SUPPORTS_PANIC_SHELL=1

# do not need to implement any cache ops
GLOBAL_DEFINES += RISCV_NO_CACHE_OPS=1

include make/module.mk
