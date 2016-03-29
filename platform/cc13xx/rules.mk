LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-m3

MEMBASE := 0x20000000
ROMBASE := 0x00000000
MEMSIZE := 0x5000

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/vectab.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/radio.c

LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	arch/arm/arm-m/systick \
	platform/cc13xx/cc13xxware

GLOBAL_COMPILEFLAGS += -DWITH_NO_FP=1
#GLOBAL_COMPILEFLAGS +=  -DDISABLE_DEBUG_OUTPUT=1

include make/module.mk

