LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/heap \
	lib/io

MODULE_SRCS += \
	$(LOCAL_DIR)/abort.c \
	$(LOCAL_DIR)/atexit.c \
	$(LOCAL_DIR)/atoi.c \
	$(LOCAL_DIR)/bsearch.c \
	$(LOCAL_DIR)/ctype.c \
	$(LOCAL_DIR)/eabi.c \
	$(LOCAL_DIR)/errno.c \
	$(LOCAL_DIR)/qsort.c \
	$(LOCAL_DIR)/rand.c \
	$(LOCAL_DIR)/stdio.c \
	$(LOCAL_DIR)/strtol.c \
	$(LOCAL_DIR)/strtoll.c \

MODULE_FLOAT_SRCS += \
	$(LOCAL_DIR)/printf.c \
	$(LOCAL_DIR)/atof.c \

MODULE_COMPILEFLAGS += -fno-builtin

MODULE_OPTIONS := extra_warnings

ifeq ($(call TOBOOL,$(WITH_TESTS)),true)
MODULE_DEPS += $(LOCAL_DIR)/test
endif

include $(LOCAL_DIR)/string/rules.mk

include make/module.mk
