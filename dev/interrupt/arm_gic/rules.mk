LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GIC_VERSION ?= 2

MODULE_DEFINES += \
	GIC_VERSION=$(GIC_VERSION) \

MODULE_SRCS += \
	$(LOCAL_DIR)/arm_gic.c

MODULE_COMPILEFLAGS += -Wno-type-limits

# Build gic_v3 for versions 3 and 4
ifeq (,$(filter-out 3 4,$(GIC_VERSION)))
MODULE_SRCS += $(LOCAL_DIR)/gic_v3.c
endif

include make/module.mk
