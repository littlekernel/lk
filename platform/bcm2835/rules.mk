LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a7
WITH_SMP := 1
SMP_CPU_ID_BITS := 8

MODULE_DEPS := \
	dev/timer/arm_generic \
	lib/cbuf

#lib/bio \
	lib/cbuf \
	lib/minip \
	dev/interrupt/arm_gic \
	dev/timer/arm_cortex_a9

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/intc.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/uart.c \

# default to no sdram unless the target calls it out
ZYNQ_SDRAM_SIZE ?= 0

MEMBASE := 0x00000000
MEMSIZE ?= 0x10000000 # 256MB
KERNEL_LOAD_OFFSET := 0x00008000 # loaded 32KB into physical

# put our kernel at 0x80000000
KERNEL_BASE = 0x80000000

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE) \
	ARM_ARCH_WAIT_FOR_SECONDARIES=1

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
