LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := riscv
SUBARCH ?= 32
RISCV_MODE ?= machine
WITH_SMP ?= 1
SMP_MAX_CPUS ?= 8

ifeq ($(RISCV_MODE),supervisor)
ifeq ($(SUBARCH),32)
RISCV_MMU ?= sv32
else
RISCV_MMU ?= sv48
endif
endif

MODULE_DEPS += lib/cbuf
MODULE_DEPS += lib/fdt
MODULE_DEPS += lib/fdtwalk
MODULE_DEPS += dev/virtio/block
MODULE_DEPS += dev/virtio/gpu
MODULE_DEPS += dev/virtio/net

MODULE_SRCS += $(LOCAL_DIR)/platform.c
MODULE_SRCS += $(LOCAL_DIR)/plic.c
MODULE_SRCS += $(LOCAL_DIR)/uart.c

MEMBASE ?= 0x80000000
MEMSIZE ?= 0x01000000 # default to 16MB
ifeq ($(RISCV_MODE),supervisor)
# offset the kernel to account for OpenSBI using the bottom
KERNEL_LOAD_OFFSET ?= 0x00200000 # kernel load offset
endif

# sifive_e or _u?
GLOBAL_DEFINES += PLATFORM_${VARIANT}=1

# set some global defines based on capability
GLOBAL_DEFINES += ARCH_RISCV_CLINT_BASE=0x02000000
GLOBAL_DEFINES += ARCH_RISCV_MTIME_RATE=10000000

# we're going to read the default memory map from a FDT
GLOBAL_DEFINES += NOVM_DEFAULT_ARENA=0

# we can revert to a poll based uart spin routine
GLOBAL_DEFINES += PLATFORM_SUPPORTS_PANIC_SHELL=1

# do not need to implement any cache ops
# (for now, since there are no hw accellerated qemu machines)
GLOBAL_DEFINES += RISCV_NO_CACHE_OPS=1

include make/module.mk
