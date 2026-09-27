LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ESWIN EIC7700X: 4x SiFive P550 (rv64gc + zba zbb), booted in supervisor mode by OpenSBI + U-Boot
ARCH := riscv
SUBARCH := 64
RISCV_MODE := supervisor
WITH_SMP ?= true
SMP_MAX_CPUS ?= 4
LK_HEAP_IMPLEMENTATION ?= dlmalloc
RISCV_FPU := true
# The P550 implements sv48, whose 512GB physmap covers the peripherals at 0 and
# either DRAM size from 2GB with room to spare. sv39 (64GB physmap) also works:
# make hifive-premier-p550-test RISCV_MMU=sv39
RISCV_MMU ?= sv48
RISCV_EXTENSION_LIST ?= zba zbb

MODULE_DEPS += lib/cbuf
MODULE_DEPS += lib/cmdline
MODULE_DEPS += lib/fdt
MODULE_DEPS += lib/fdtwalk
MODULE_DEPS += dev/interrupt/riscv_plic
MODULE_DEPS += dev/uart/dwc8250

MODULE_SRCS += $(LOCAL_DIR)/platform.c
MODULE_SRCS += $(LOCAL_DIR)/uart.c

MEMBASE ?= 0x80000000
MEMSIZE ?= 0x400000000 # 16GB, the smaller of the two board configurations
ifeq ($(RISCV_MODE),supervisor)
# OpenSBI owns the first 384KB of DRAM; 0x80200000 is also U-Boot's default load address
KERNEL_LOAD_OFFSET ?= 0x00200000
endif

# mtime ticks at 1MHz
GLOBAL_DEFINES += ARCH_RISCV_MTIME_RATE=1000000

# we can revert to a poll based uart spin routine
GLOBAL_DEFINES += PLATFORM_SUPPORTS_PANIC_SHELL=1

# coherent L3, no cache maintenance needed
GLOBAL_DEFINES += RISCV_NO_CACHE_OPS=1

include make/module.mk
