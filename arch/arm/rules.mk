LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# can override this in local.mk
ENABLE_THUMB?=true

# default to the regular arm subarch
SUBARCH := arm

DEFINES += \
	ARM_CPU_$(ARM_CPU)=1

# do set some options based on the cpu core
HANDLED_CORE := false
ifeq ($(ARM_CPU),cortex-m3)
DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_ISA_ARMv7=1 \
	ARM_ISA_ARMv7M=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
ENABLE_THUMB := true
ONLY_THUMB := true
SUBARCH := arm-m
endif
ifeq ($(ARM_CPU),cortex-a8)
DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv7=1 \
	ARM_ISA_ARMv7A=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_NEON=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1 \
	ARM_WITH_L2=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
#CFLAGS += -mfpu=neon -mfloat-abi=softfp
endif
ifeq ($(ARM_CPU),arm1136j-s)
DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv6=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_CACHE=1 \
	ARM_CPU_ARM1136=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),arm1176jzf-s)
DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv6=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_CACHE=1 \
	ARM_CPU_ARM1136=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),arm926ej-s)
DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv5E=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_CACHE=1 \
	ARM_CPU_ARM9=1 \
	ARM_CPU_ARM926=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),arm7tdmi)
DEFINES += \
	ARM_ISA_ARMv4=1 \
	ARM_WITH_THUMB=1 \
	ARM_CPU_ARM7=1
GLOBAL_COMPILEFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
endif

ifneq ($(HANDLED_CORE),true)
$(warning $(LOCAL_DIR)/rules.mk doesnt have logic for arm core $(ARM_CPU))
$(warning this is likely to be broken)
endif

THUMBCFLAGS :=
THUMBINTERWORK :=
ifeq ($(ENABLE_THUMB),true)
THUMBCFLAGS := -mthumb -D__thumb__
THUMBINTERWORK := -mthumb-interwork
endif

INCLUDES += \
	-I$(LOCAL_DIR)/include \
	-I$(LOCAL_DIR)/$(SUBARCH)/include

ifeq ($(SUBARCH),arm)
MODULE_SRCS += \
	$(LOCAL_DIR)/arm/start.S \
	$(LOCAL_DIR)/arm/asm.S \
	$(LOCAL_DIR)/arm/cache-ops.S \
	$(LOCAL_DIR)/arm/cache.c \
	$(LOCAL_DIR)/arm/ops.S \
	$(LOCAL_DIR)/arm/exceptions.S \
	$(LOCAL_DIR)/arm/faults.c \
	$(LOCAL_DIR)/arm/mmu.c \
	$(LOCAL_DIR)/arm/thread.c \
	$(LOCAL_DIR)/arm/dcc.S

MODULE_ARM_OVERRIDE_SRCS := \
	$(LOCAL_DIR)/arm/arch.c

DEFINES += \
	ARCH_DEFAULT_STACK_SIZE=4096
endif
ifeq ($(SUBARCH),arm-m)
MODULE_SRCS += \
	$(LOCAL_DIR)/arm-m/arch.c \
	$(LOCAL_DIR)/arm-m/vectab.c \
	$(LOCAL_DIR)/arm-m/start.c \
	$(LOCAL_DIR)/arm-m/exceptions.c \
	$(LOCAL_DIR)/arm-m/thread.c \
	$(LOCAL_DIR)/arm-m/systick.c

INCLUDES += \
	-I$(LOCAL_DIR)/arm-m/CMSIS/Include

DEFINES += \
	ARCH_DEFAULT_STACK_SIZE=1024
endif

# set the default toolchain to arm elf and set a #define
TOOLCHAIN_PREFIX ?= arm-elf-
ifeq ($(TOOLCHAIN_PREFIX),arm-none-linux-gnueabi-)
# XXX test for EABI better than this
# eabi compilers dont need this
THUMBINTERWORK:=
endif

GLOBAL_COMPILEFLAGS += $(THUMBINTERWORK)

# make sure some bits were set up
MEMVARS_SET := 0
ifneq ($(MEMBASE),)
MEMVARS_SET := 1
endif
ifneq ($(MEMSIZE),)
MEMVARS_SET := 1
endif
ifeq ($(MEMVARS_SET),0)
$(error missing MEMBASE or MEMSIZE variable, please set in target rules.mk)
endif

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(THUMBCFLAGS) -print-libgcc-file-name)
$(info LIBGCC = $(LIBGCC))

$(info GLOBAL_COMPILEFLAGS = $(GLOBAL_COMPILEFLAGS) $(THUMBCFLAGS))

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/system-onesegment.ld \
	$(BUILDDIR)/system-twosegment.ld

# rules for generating the linker scripts

$(BUILDDIR)/system-onesegment.ld: $(LOCAL_DIR)/system-onesegment.ld
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/" < $< > $@

$(BUILDDIR)/system-twosegment.ld: $(LOCAL_DIR)/system-twosegment.ld
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%ROMBASE%/$(ROMBASE)/;s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/" < $< > $@

# arm specific script to try to guess stack usage
$(OUTELF).stack: LOCAL_DIR:=$(LOCAL_DIR)
$(OUTELF).stack: $(OUTELF).lst
	$(NOECHO)echo generating stack usage $@
	$(NOECHO)$(LOCAL_DIR)/stackusage < $< | sort -n -k 1 -r > $@

EXTRA_BUILDDEPS += $(OUTELF).stack

include make/module.mk
