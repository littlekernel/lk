LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a9-neon

MODULE_DEPS := \
	lib/cbuf \
	dev/cache/pl310 \
	dev/interrupt/arm_gic \
	dev/timer/arm_cortex_a9

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/clocks.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/fpga.c

# default to no sdram unless the target calls it out
ZYNQ_SDRAM_SIZE ?= 0

ifeq ($(ZYNQ_USE_SRAM),1)
MEMBASE := 0x0
MEMSIZE := 0x30000 # 3 * 64K
else
# XXX untested path
MEMBASE := 0x00000000
MEMSIZE ?= $(ZYNQ_SDRAM_SIZE) # 256MB
#KERNEL_LOAD_OFFSET := 0x00100000 # loaded 1MB into physical space
endif

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
