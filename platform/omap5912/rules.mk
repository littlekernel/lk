LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := arm
ARM_CPU := arm926ej-s
CPU := generic

MODULES += \
	lib/cbuf

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/timer.o


#	$(LOCAL_DIR)/console.o \

MEMBASE := 0x10000000
#MEMSIZE := 0x02000000	# 32MB

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

