LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := x86
CPU := generic

MODULES += \
	lib/cbuf

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/console.o \
	$(LOCAL_DIR)/keyboard.o \
	$(LOCAL_DIR)/pci.o

LINKER_SCRIPT += \
	$(BUILDDIR)/kernel.ld

