LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
  lib/minip \

MODULE_SRCS += \
  $(LOCAL_DIR)/tftp.c \

include make/module.mk
