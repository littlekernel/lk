LOCAL_DIR := $(GET_LOCAL_DIR)

# can override this in local.mk
ENABLE_THUMB?=true

DEFINES += \
	ARM_CPU_$(ARM_CPU)=1

# do set some options based on the cpu core
HANDLED_CORE := false
ifeq ($(ARM_CPU),cortex-a8)
DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv7=1 \
	ARM_WITH_VFP=1 \
	ARM_WITH_NEON=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_THUMB2=1 \
	ARM_WITH_CACHE=1 \
	ARM_WITH_L2=1
CFLAGS += -mcpu=$(ARM_CPU)
#CFLAGS += -mcpu=arm1136jf-s # compiler doesn't understand cortex yet
HANDLED_CORE := true
#CFLAGS += -mfpu=vfp -mfloat-abi=softfp
endif
ifeq ($(ARM_CPU),arm1136j-s)
DEFINES += \
	ARM_WITH_CP15=1 \
	ARM_WITH_MMU=1 \
	ARM_ISA_ARMv6=1 \
	ARM_WITH_THUMB=1 \
	ARM_WITH_CACHE=1 \
	ARM_CPU_ARM1136=1
CFLAGS += -mcpu=$(ARM_CPU)
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
CFLAGS += -mcpu=$(ARM_CPU)
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
CFLAGS += -mcpu=$(ARM_CPU)
HANDLED_CORE := true
endif
ifeq ($(ARM_CPU),arm7tdmi)
DEFINES += \
	ARM_ISA_ARMv4=1 \
	ARM_WITH_THUMB=1 \
	ARM_CPU_ARM7=1
CFLAGS += -mcpu=$(ARM_CPU)
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
	-I$(LOCAL_DIR)/include

BOOTOBJS += \
	$(LOCAL_DIR)/crt0.o

OBJS += \
	$(LOCAL_DIR)/arch.Ao \
	$(LOCAL_DIR)/asm.o \
	$(LOCAL_DIR)/cache.o \
	$(LOCAL_DIR)/cache-ops.o \
	$(LOCAL_DIR)/ops.o \
	$(LOCAL_DIR)/exceptions.o \
	$(LOCAL_DIR)/faults.o \
	$(LOCAL_DIR)/mmu.o \
	$(LOCAL_DIR)/thread.o

# set the default toolchain to arm elf and set a #define
TOOLCHAIN_PREFIX ?= arm-elf-
ifeq ($(TOOLCHAIN_PREFIX),arm-none-linux-gnueabi-)
# XXX test for EABI better than this
# eabi compilers dont need this
THUMBINTERWORK:=
else

# XXX hack to work around lack of cortex support in regular compilers
CFLAGS := $(subst cortex-a8,arm1136jf-s,$(CFLAGS))
endif

CFLAGS += $(THUMBINTERWORK)

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

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(CFLAGS) $(THUMBCFLAGS) -print-libgcc-file-name)
#$(info LIBGCC = $(LIBGCC))

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

