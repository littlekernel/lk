LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ifeq ($(ARCH),)
ARCH := arm64
endif
ifeq ($(ARCH),arm64)
ARM_CPU ?= cortex-a53
endif
ifeq ($(ARCH),arm)
ARM_CPU ?= cortex-a15
endif
WITH_SMP ?= 1

LK_HEAP_IMPLEMENTATION ?= dlmalloc

MODULE_SRCS += $(LOCAL_DIR)/debug.c
MODULE_SRCS += $(LOCAL_DIR)/platform.c

MEMBASE := 0x40000000
MEMSIZE ?= 0x08000000   # 512MB
KERNEL_LOAD_OFFSET := 0x100000 # 1MB

MODULE_DEPS += \
    dev/bus/pci \
    dev/bus/pci/drivers \
    dev/interrupt/arm_gic \
    dev/power/psci \
    dev/timer/arm_generic \
    dev/uart/pl011 \
    dev/virtio/9p \
    dev/virtio/block \
    dev/virtio/gpu \
    dev/virtio/net \
    lib/cbuf \
    lib/fdtwalk \
    lib/fs/9p \

GLOBAL_DEFINES += \
    MEMBASE=$(MEMBASE) \
    MEMSIZE=$(MEMSIZE) \
    PLATFORM_SUPPORTS_PANIC_SHELL=1 \
    CONSOLE_HAS_INPUT_BUFFER=1 \
    TIMER_ARM_GENERIC_SELECTED=CNTV

GLOBAL_DEFINES += MMU_WITH_TRAMPOLINE=1

LINKER_SCRIPT += \
    $(BUILDDIR)/system-onesegment.ld

include make/module.mk
