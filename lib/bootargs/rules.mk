LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \

MODULE_SRCS := \
    $(LOCAL_DIR)/bootargs.c

include make/module.mk
