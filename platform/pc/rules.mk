LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

CPU := generic

MODULE_DEPS += \
    lib/cbuf \

MODULE_SRCS += \
    $(LOCAL_DIR)/interrupts.c \
    $(LOCAL_DIR)/platform.c \
    $(LOCAL_DIR)/timer.c \
    $(LOCAL_DIR)/debug.c \
    $(LOCAL_DIR)/console.c \
    $(LOCAL_DIR)/keyboard.c \
    $(LOCAL_DIR)/pci.c \
    $(LOCAL_DIR)/ide.c \
    $(LOCAL_DIR)/uart.c \


LK_HEAP_IMPLEMENTATION ?= dlmalloc

include make/module.mk

