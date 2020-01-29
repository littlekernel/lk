LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

WITH_SMP := 1
#LK_HEAP_IMPLEMENTATION ?= dlmalloc

# 1st pass to set arch
ifeq ($(TARGET),rpi2)
  ARCH := arm
  ARM_CPU := cortex-a7
  GLOBAL_DEFINES += CRYSTAL=19200000
else ifeq ($(TARGET),rpi3)
  ARCH := arm64
  ARM_CPU := cortex-a53
  GLOBAL_DEFINES += CRYSTAL=19200000
else ifeq ($(TARGET),rpi3-vpu)
  GLOBAL_DEFINES += CRYSTAL=19200000
else ifeq ($(TARGET),rpi4-vpu)
  ARCH ?= vc4
  GLOBAL_DEFINES += CRYSTAL=54000000
endif


ifeq ($(ARCH),arm)
MODULE_DEPS += \
	dev/timer/arm_generic \
	lib/cbuf
MODULE_SRCS += \
	$(LOCAL_DIR)/mailbox.c \
	$(LOCAL_DIR)/intc.c \

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld
else # VPU
  ifeq ($(BOOTCODE),1)
    LINKER_SCRIPT += $(LOCAL_DIR)/bootcode.ld
  else
    LINKER_SCRIPT += $(LOCAL_DIR)/start.ld
  endif
endif


#lib/bio \
	lib/cbuf \
	lib/minip \
	dev/interrupt/arm_gic \
	dev/timer/arm_cortex_a9

MODULE_SRCS += \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/pll_read.c \


MEMBASE := 0x00000000

GLOBAL_DEFINES += \
	ARM_ARCH_WAIT_FOR_SECONDARIES=1

ifeq ($(TARGET),rpi2)
# put our kernel at 0x80000000
KERNEL_BASE = 0x80000000
KERNEL_LOAD_OFFSET := 0x00008000
MEMSIZE ?= 0x10000000 # 256MB
SMP_CPU_ID_BITS := 8
GLOBAL_DEFINES += \
	BCM2836=1

MODULE_SRCS += \
	$(LOCAL_DIR)/uart.c

else ifeq ($(TARGET),rpi3)

KERNEL_LOAD_OFFSET := 0x00080000
MEMSIZE ?= 0x40000000 # 1GB

GLOBAL_DEFINES += \
    MEMBASE=$(MEMBASE) \
    MEMSIZE=$(MEMSIZE) \
    MMU_WITH_TRAMPOLINE=1 \
	BCM2837=1

MODULE_SRCS += \
	$(LOCAL_DIR)/miniuart.c

MODULE_DEPS += \
		app/shell \
	    app/tests \
	    lib/fdt
else ifeq ($(TARGET),rpi3-vpu)
  MEMSIZE := 0x20000 # 128kb
  MEMBASE := 0x80000000 # in the 8 alias
  GLOBAL_DEFINES += \
    MEMSIZE=$(MEMSIZE) \
    MEMBASE=$(MEMBASE) \
    RPI3=1 \
    VPU=1 \
    SMP_MAX_CPUS=1 \

  MODULE_SRCS += \
    $(LOCAL_DIR)/uart.c \

else ifeq ($(TARGET),rpi4-vpu)
MEMSIZE ?= 0x01400000 # 20MB
MEMBASE := 0xc0000000
GLOBAL_DEFINES += \
    BCM2XXX_VPU=1 SMP_MAX_CPUS=1 \
    MEMSIZE=$(MEMSIZE) \
    MEMBASE=$(MEMBASE) \
    RPI4=1 \
    VPU=1 \

MODULE_SRCS += \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/genet.c \

endif

include make/module.mk
