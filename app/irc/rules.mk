LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/irc.cpp \

MODULE_DEPS := \
    lib/libcpp \
    lib/minip

include make/module.mk
