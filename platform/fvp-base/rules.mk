LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

LK_HEAP_IMPLEMENTATION ?= dlmalloc

ARCH	:= arm64
ARM_CPU := cortex-a53
CPU	:= generic
WITH_SMP := 1

MODULE_SRCS += $(LOCAL_DIR)/debug.c
MODULE_SRCS += $(LOCAL_DIR)/platform.c

MEMBASE := 0x80000000
MEMSIZE ?= 0x80000000
KERNEL_LOAD_OFFSET := 0x8000000

MODULE_DEPS += \
    dev/power/psci \
    dev/interrupt/arm_gic \
    dev/timer/arm_generic \
    dev/uart/pl011 \
    lib/fdtwalk \

GLOBAL_DEFINES += \
    MEMBASE=$(MEMBASE) \
    MEMSIZE=$(MEMSIZE) \
    MMU_WITH_TRAMPOLINE=1 \
    TIMER_ARM_GENERIC_SELECTED=CNTV

LINKER_SCRIPT += \
    $(BUILDDIR)/system-onesegment.ld

include make/module.mk
