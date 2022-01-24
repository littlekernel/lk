LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# two implementations, modern and legacy
# legacy implies older hardware, pre pentium, pre pci
CPU ?= modern

MODULE_DEPS += \
    lib/acpi_lite \
    lib/bio \
    lib/cbuf

ifneq ($(CPU),legacy)
MODULE_DEPS += dev/bus/pci/drivers
endif

MODULE_SRCS += \
    $(LOCAL_DIR)/cmos.c \
    $(LOCAL_DIR)/console.c \
    $(LOCAL_DIR)/debug.c \
    $(LOCAL_DIR)/ide.c \
    $(LOCAL_DIR)/interrupts.c \
    $(LOCAL_DIR)/keyboard.c \
    $(LOCAL_DIR)/lapic.c \
    $(LOCAL_DIR)/pic.c \
    $(LOCAL_DIR)/platform.c \
    $(LOCAL_DIR)/timer.c \
    $(LOCAL_DIR)/uart.c \

LK_HEAP_IMPLEMENTATION ?= dlmalloc

include make/module.mk

