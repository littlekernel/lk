LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a9-neon
WITH_SMP := 1

MODULE_DEPS := \
	lib/bio \
	lib/cbuf \
	lib/minip \
	dev/cache/pl310 \
	dev/interrupt/arm_gic \
	dev/timer/arm_cortex_a9


GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/clocks.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/fpga.c \
	$(LOCAL_DIR)/gem.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/qspi.c \
	$(LOCAL_DIR)/spiflash.c \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/uart.c \

# default to no sdram unless the target calls it out
ZYNQ_SDRAM_SIZE ?= 0

ifeq ($(ZYNQ_USE_SRAM),1)
MEMBASE := 0x0
MEMSIZE := 0x30000 # 3 * 64K

GLOBAL_DEFINES += \
	ZYNQ_CODE_IN_SRAM=1 \
	ZYNQ_SDRAM_INIT=1
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
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE) \
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
