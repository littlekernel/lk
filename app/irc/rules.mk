LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_OPTIONS := extra_warnings

MODULE_SRCS += \
	$(LOCAL_DIR)/irc.cpp \

MODULE_DEPS := \
    lib/libcpp \
    lib/minip \
    lib/version

include make/module.mk
