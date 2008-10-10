LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a8
CPU := generic

DEVS += usb

# provides a few devices
DEFINES += \
	WITH_DEV_USBC=1 \
	WITH_DEV_UART=1

MODULES += \
	dev/usb

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/cpu_early_init.Ao \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/i2c.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/uart.o \
	$(LOCAL_DIR)/usbc.o

MEMBASE := 0x80000000

DEFINES += MEMBASE=$(MEMBASE) \
	WITH_CPU_EARLY_INIT=1

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

