LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/inc

MODULE_SRCS += $(LOCAL_DIR)/system_stm32h5xx.c

# stm32h5xx.h includes math.h, which SystemCoreClockUpdate() uses for float_t
MODULE_DEPS += lib/libm

include make/module.mk
