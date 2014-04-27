LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a9

MODULE_DEPS := \
	lib/cbuf \
	dev/interrupt/arm_gic

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/clocks.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/uart.c

ifeq ($(ZYNQ_USE_SRAM),1)
MEMBASE := 0x0
MEMSIZE ?= 0x40000 # 256KB
else
MEMBASE := 0x0
MEMSIZE ?= 0x10000000	# 256MB
endif

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE)

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
