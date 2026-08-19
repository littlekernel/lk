LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/spifstest.c

MODULE_DEPS += \
    lib/bio \
    lib/fs/spifs \
    lib/libm \
    lib/unittest

include make/module.mk
