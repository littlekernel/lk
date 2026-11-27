LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/arm_gic.c
MODULE_SRCS += $(LOCAL_DIR)/gic_v2.c
MODULE_SRCS += $(LOCAL_DIR)/gic_v2m.c
MODULE_SRCS += $(LOCAL_DIR)/gic_v3.c

# LPIs and the ITS: arm64 only, they need 64 bit register access
ifeq ($(ARCH),arm64)
MODULE_SRCS += $(LOCAL_DIR)/gic_v3_its.c
MODULE_SRCS += $(LOCAL_DIR)/gic_v3_lpi.c
endif

MODULE_OPTIONS += extra_warnings test

include make/module.mk
