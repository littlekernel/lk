LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

GLOBAL_DEFINES += \
	PLATFORM_HAS_DYNAMIC_TIMER=1

MODULE_DEPS += \
    lib/fixed_point

MODULE_SRCS += \
	$(LOCAL_DIR)/arm_cortex_a9_timer.c

include make/module.mk
