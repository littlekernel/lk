LOCAL_DIR := $(GET_LOCAL_DIR)


#CFLAGS += -mcpu=$(ARM_CPU) -finline
CFLAGS += -ml -m4-single-only -mhitachi

DEFINES += \
	ARM_CPU_$(ARM_CPU)=1

INCLUDES += \
	-I$(LOCAL_DIR)/include

BOOTOBJS += \
	$(LOCAL_DIR)/crt0.o

OBJS += \
	$(LOCAL_DIR)/asm.o \
	$(LOCAL_DIR)/arch.o \
	$(LOCAL_DIR)/thread.o

#	$(LOCAL_DIR)/arch.o \
	$(LOCAL_DIR)/asm.o \
	$(LOCAL_DIR)/cache.o \
	$(LOCAL_DIR)/ops.o \
	$(LOCAL_DIR)/exceptions.o \
	$(LOCAL_DIR)/faults.o \
	$(LOCAL_DIR)/mmu.o \
	$(LOCAL_DIR)/thread.o

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

