LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := x86
CPU := generic

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/console.o \
	$(LOCAL_DIR)/keyboard.o

MEMBASE := 0x0
MEMSIZE := 0x400000	# 4MB

LINKER_SCRIPT += \
	$(BUILDDIR)/kernel.ld

