LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
  $(LOCAL_DIR)/psci.c \
  $(LOCAL_DIR)/psci_asm.S \

include make/module.mk

