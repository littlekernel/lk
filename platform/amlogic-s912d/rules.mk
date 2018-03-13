LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
    lib/cbuf \
    lib/fdt \
    dev/interrupt/arm_gic \
    dev/timer/arm_generic \

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/uart.c \

ARCH := arm64
ARM_CPU := cortex-a53
MEMBASE := 0
MEMSIZE := 0x80000000	# 2GB
KERNEL_LOAD_OFFSET := 0x01080000
LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
		app/shell \


WITH_CPP_SUPPORT=true

include make/module.mk