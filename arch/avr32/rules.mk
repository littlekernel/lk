LOCAL_DIR := $(GET_LOCAL_DIR)

DEFINES += \
	AVR32_CPU_$(AVR32_CPU)=1

INCLUDES += \
	-I$(LOCAL_DIR)/include

BOOTOBJS += \
	$(LOCAL_DIR)/crt0.o

OBJS += \
	$(LOCAL_DIR)/ops.o \
	$(LOCAL_DIR)/thread.o \
	$(LOCAL_DIR)/arch.o \
	$(LOCAL_DIR)/cache.o \

#	$(LOCAL_DIR)/asm.o \
	$(LOCAL_DIR)/cache-ops.o \
	$(LOCAL_DIR)/exceptions.o \
	$(LOCAL_DIR)/faults.o \
	$(LOCAL_DIR)/mmu.o \
	$(LOCAL_DIR)/dcc.o

# set the default toolchain and set a #define
TOOLCHAIN_PREFIX ?= avr32-unknown-none-

CFLAGS += -mcpu=ap7000
LDFLAGS += --relax

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

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(CFLAGS) -print-libgcc-file-name)
#$(info LIBGCC = $(LIBGCC))

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/system-onesegment.ld \
	$(BUILDDIR)/system-twosegment.ld

# rules for generating the linker scripts
PROGBASE ?= $(MEMBASE)

$(BUILDDIR)/system-onesegment.ld: $(LOCAL_DIR)/system-onesegment.ld
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%PROGBASE%/$(PROGBASE)/" < $< > $@

$(BUILDDIR)/system-twosegment.ld: $(LOCAL_DIR)/system-twosegment.ld
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%ROMBASE%/$(ROMBASE)/;s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%PROGBASE%/$(PROGBASE)/" < $< > $@

