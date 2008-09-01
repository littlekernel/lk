LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := arm
ARM_CPU := arm926ej-s
CPU := generic

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/timer.o \

#	$(LOCAL_DIR)/net.o \


#	$(LOCAL_DIR)/console.o \

MEMBASE ?= 0x0
MEMSIZE ?= 0x08000000	# 128MB

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

