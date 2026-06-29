LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/cmpctmalloc.c

# enable tests to be build as a submodule
MODULE_OPTIONS += test

include make/module.mk
