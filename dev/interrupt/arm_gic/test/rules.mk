LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# the ITS and LPIs are arm64 only
ifeq ($(ARCH),arm64)
MODULE_SRCS += $(LOCAL_DIR)/its_tests.c
endif

MODULE_DEPS += \
    dev/interrupt/arm_gic \
    lib/unittest

include make/module.mk
