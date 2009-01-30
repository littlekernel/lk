LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a8
#arm1136j-s
CPU := generic

DEFINES += WITH_CPU_EARLY_INIT=1 MEMBASE=0

INCLUDES += -I$(LOCAL_DIR)/include

DEVS += fbcon
MODULES += dev/fbcon

OBJS += \
	$(LOCAL_DIR)/arch_init.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/gpio.o \
	$(LOCAL_DIR)/lcdc.o

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld

include platform/msm_shared/rules.mk

