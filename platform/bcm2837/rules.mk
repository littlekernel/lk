LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm64
ARM_CPU := cortex-a53
WITH_SMP := 1
#LK_HEAP_IMPLEMENTATION ?= dlmalloc
WITH_CPP_SUPPORT=true

MODULE_DEPS := \
	dev/timer/arm_generic \
	lib/cbuf \
	app/shell \
    app/tests \
    lib/fdt \

#lib/bio \
	lib/cbuf \
	lib/minip \
	dev/interrupt/arm_gic \
	dev/timer/arm_cortex_a9

MODULE_SRCS += \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/intc.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/uart.c \

MEMBASE := 0x00000000
MEMSIZE ?= 0x40000000 # 256MB
KERNEL_LOAD_OFFSET := 0x00080000 



# put our kernel at 0x80000000
#KERNEL_BASE = 0xFFFF000000080000  

GLOBAL_DEFINES += \
    MEMBASE=$(MEMBASE) \
    MEMSIZE=$(MEMSIZE) \
    MMU_WITH_TRAMPOLINE=1 \
	ARM_ARCH_WAIT_FOR_SECONDARIES=1

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
