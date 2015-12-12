LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \

MODULE_SRCS := \
    $(LOCAL_DIR)/null.cpp

ifeq ($(WITH_CPP_SUPPORT),true)
MODULE_SRCS += \

endif

include make/module.mk
