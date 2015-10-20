LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a9-neon
WITH_SMP ?= 1
SMP_MAX_CPUS := 2

MODULE_DEPS := \
	lib/bio \
	lib/cbuf \
	lib/watchdog \
	dev/cache/pl310 \
	dev/interrupt/arm_gic \
	dev/timer/arm_cortex_a9

MODULE_SRCS += \
	$(LOCAL_DIR)/clocks.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/fpga.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/qspi.c \
	$(LOCAL_DIR)/spiflash.c \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/swdt.c \
	$(LOCAL_DIR)/uart.c \

# default to no sdram unless the target calls it out
ZYNQ_SDRAM_SIZE ?= 0

# default to having the gem ethernet controller
ZYNQ_WITH_GEM_ETH ?= 1

ifeq ($(ZYNQ_WITH_GEM_ETH),1)
MODULE_SRCS += \
	$(LOCAL_DIR)/gem.c \

GLOBAL_DEFINES += \
	ZYNQ_WITH_GEM_ETH=1 \
	ARM_ARCH_WAIT_FOR_SECONDARIES=1

# gem driver depends on minip interface
MODULE_DEPS += \
	lib/minip
endif

ifeq ($(ZYNQ_USE_SRAM),1)
MEMBASE := 0x0
MEMSIZE := 0x40000 # 4 * 64K

GLOBAL_DEFINES += \
	ZYNQ_CODE_IN_SRAM=1

ifneq ($(ZYNQ_SDRAM_SIZE),0)
GLOBAL_DEFINES += \
	ZYNQ_SDRAM_INIT=1
endif

else
MEMBASE := 0x00000000
MEMSIZE ?= $(ZYNQ_SDRAM_SIZE) # 256MB
KERNEL_LOAD_OFFSET := 0x00100000 # loaded 1MB into physical space

# set a #define so system code can decide if it needs to reinitialize dram or not
GLOBAL_DEFINES += \
	ZYNQ_CODE_IN_SDRAM=1
endif

# put our kernel at 0xc0000000 so we can have axi bus 1 mapped at 0x80000000
KERNEL_BASE = 0xc0000000

GLOBAL_DEFINES += \
	SDRAM_SIZE=$(ZYNQ_SDRAM_SIZE)

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

# python script to generate the zynq's bootrom bootheader
BOOTHEADERBIN := $(BUILDDIR)/BOOT.BIN
MKBOOTHEADER := $(LOCAL_DIR)/mkbootheader.py
EXTRA_BUILDDEPS += $(BOOTHEADERBIN)
GENERATED += $(BOOTHEADERBIN)

$(BOOTHEADERBIN): $(OUTBIN) $(MKBOOTHEADER)
	@$(MKDIR)
	$(NOECHO)echo generating $@; \
	$(MKBOOTHEADER) $(OUTBIN) $@

include make/module.mk
