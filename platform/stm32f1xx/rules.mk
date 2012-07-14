LOCAL_DIR := $(GET_LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x0
MEMBASE := 0x20000000
# can be overridden by target

ARCH := arm
ARM_CPU := cortex-m3

ifeq ($(STM32_CHIP),stm32f107)
DEFINES += \
	STM32F10X_CL=1	
MEMSIZE ?= 65536
endif
ifeq ($(STM32_CHIP),stm32f103_xl)
DEFINES += \
	STM32F10X_XL=1
MEMSIZE ?= 65536
endif
ifeq ($(STM32_CHIP),stm32f103_hd)
DEFINES += \
	STM32F10X_HD=1
MEMSIZE ?= 65536
endif
ifeq ($(STM32_CHIP),stm32f103_md)
DEFINES += \
	STM32F10X_MD=1
MEMSIZE ?= 20480
endif
ifeq ($(STM32_CHIP),stm32f103_ld)
DEFINES += \
	STM32F10X_LD=1
MEMSIZE ?= 20480
endif

DEFINES += \
	MEMSIZE=$(MEMSIZE)

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/init.o \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/vectab.o \

#	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/platform_early.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/init_clock.o \
	$(LOCAL_DIR)/init_clock_48mhz.o \
	$(LOCAL_DIR)/mux.o \
	$(LOCAL_DIR)/emac_dev.o

# use a two segment memory layout, where all of the read-only sections 
# of the binary reside in rom, and the read/write are in memory. The 
# ROMBASE, MEMBASE, and MEMSIZE make variables are required to be set 
# for the linker script to be generated properly.
#
LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULES += \
	lib/cbuf

include $(LOCAL_DIR)/STM32F10x_StdPeriph_Driver/rules.mk $(LOCAL_DIR)/CMSIS/rules.mk
