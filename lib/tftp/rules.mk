LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
  lib/minip \
  lib/cksum

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
  $(LOCAL_DIR)/tftp.c \

include make/module.mk
