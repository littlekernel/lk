LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/spifstest.c

MODULE_DEPS += \
    lib/libm \

include make/module.mk
