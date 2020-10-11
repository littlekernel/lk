LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

WITH_SMP := 1
#LK_HEAP_IMPLEMENTATION ?= dlmalloc

# 1st pass to set arch
ifeq ($(TARGET),rpi1)
  ARCH := arm
  ARM_CPU := arm1176jzf-s
  HAVE_ARM_TIMER = 0
else ifeq ($(TARGET),rpi2)
  ARCH := arm
  ARM_CPU := cortex-a7
  HAVE_ARM_TIMER = 1
else ifeq ($(TARGET),rpi3)
  ARCH := arm64
  ARM_CPU := cortex-a53
  HAVE_ARM_TIMER = 1
else ifeq ($(TARGET),rpi3-vpu)
  ARCH := vpu
else ifeq ($(TARGET),rpi4-vpu)
endif


ifeq ($(ARCH),vpu)
  MODULE_DEPS += platform/bcm28xx/pll
  ifeq ($(BOOTCODE),1)
    MEMBASE := 0x80000000 # in the 8 alias
    MEMSIZE := 0x20000 # 128kb
    LINKER_SCRIPT += $(LOCAL_DIR)/bootcode.ld
  else
    MEMBASE := 0xc0000000
    MEMSIZE ?= 0x01400000 # 20MB
    LINKER_SCRIPT += $(LOCAL_DIR)/start.ld
  endif
else # VPU
  ifeq ($(HAVE_ARM_TIMER),1)
    MODULE_DEPS += dev/timer/arm_generic
    GLOBAL_DEFINES += HAVE_ARM_TIMER=1
  else
    MODULE_DEPS += dev/timer/vc4
    GLOBAL_DEFINES += VC4_TIMER_CHANNEL=1
  endif
  MEMBASE := 0x00000000
  MODULE_DEPS += lib/cbuf
  MODULE_SRCS += \
    $(LOCAL_DIR)/mailbox.c \
    $(LOCAL_DIR)/intc.c \

  LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld
endif


#lib/bio \
	lib/cbuf \
	lib/minip \
	dev/interrupt/arm_gic \
	dev/timer/arm_cortex_a9

MODULE_SRCS += \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/udelay.c \
	#$(LOCAL_DIR)/i2c.c \



GLOBAL_DEFINES += \
	ARM_ARCH_WAIT_FOR_SECONDARIES=1

ifeq ($(TARGET),rpi1)
  KERNEL_BASE = 0x00000000
  MMIO_BASE_VIRT = 0x20000000U
  KERNEL_LOAD_OFFSET := 0x00000000
  MEMSIZE ?= 0x10000000 # 256MB
  WITH_SMP = 0
  GLOBAL_DEFINES += BCM2835=1 MMIO_BASE_VIRT=$(MMIO_BASE_VIRT) TARGET_HAS_DEBUG_LED=1
  MODULE_SRCS += $(LOCAL_DIR)/uart.c
else ifeq ($(TARGET),rpi2)
  # put our kernel at 0x80000000
  KERNEL_BASE = 0x80000000
  MMIO_BASE_VIRT = 0xe0000000U
  KERNEL_LOAD_OFFSET := 0x00008000
  MEMSIZE ?= 0x10000000 # 256MB
  SMP_CPU_ID_BITS := 8
  GLOBAL_DEFINES += BCM2836=1 MMIO_BASE_VIRT=$(MMIO_BASE_VIRT)

  MODULE_SRCS += $(LOCAL_DIR)/uart.c
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
  GLOBAL_DEFINES += \
    MEMSIZE=$(MEMSIZE) \
    MEMBASE=$(MEMBASE) \
    RPI3=1 \
    VPU=1 \
    SMP_MAX_CPUS=1 \

  MODULE_SRCS += \
    $(LOCAL_DIR)/uart.c \
    $(LOCAL_DIR)/sdhost_impl.cpp \
    $(LOCAL_DIR)/print_timestamp.c \

  MODULES += \
	lib/bio \

else ifeq ($(TARGET),rpi4-vpu)
GLOBAL_DEFINES += \
    BCM2XXX_VPU=1 SMP_MAX_CPUS=1 \
    MEMSIZE=$(MEMSIZE) \
    MEMBASE=$(MEMBASE) \
    RPI4=1 \
    VPU=1 \

MODULE_SRCS += \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/genet.c \
	$(LOCAL_DIR)/udelay.c \

endif

include make/module.mk
