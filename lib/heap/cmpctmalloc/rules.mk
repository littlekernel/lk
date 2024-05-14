LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/cmpctmalloc.c

# cmpct_test_trim() uses some floating point
# TODO: move tests to another file to avoid this
MODULE_OPTIONS := float

include make/module.mk
