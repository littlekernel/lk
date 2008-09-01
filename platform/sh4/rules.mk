LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := sh
SH_CPU := sh4
CPU := generic

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/timer.o


#	$(LOCAL_DIR)/console.o \

MEMBASE := 0x80c00000
MEMSIZE := 0x01000000	# 16MB

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

