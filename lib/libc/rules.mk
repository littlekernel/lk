LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/heap \
	lib/io

MODULE_SRCS += \
	$(LOCAL_DIR)/atoi.c \
	$(LOCAL_DIR)/bsearch.c \
	$(LOCAL_DIR)/ctype.c \
	$(LOCAL_DIR)/errno.c \
	$(LOCAL_DIR)/printf.c \
	$(LOCAL_DIR)/rand.c \
	$(LOCAL_DIR)/strtol.c \
	$(LOCAL_DIR)/strtoll.c \
	$(LOCAL_DIR)/stdio.c \
	$(LOCAL_DIR)/qsort.c \
	$(LOCAL_DIR)/eabi.c

ifeq ($(WITH_CPP_SUPPORT),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/atexit.c \
	$(LOCAL_DIR)/pure_virtual.cpp
endif

include $(LOCAL_DIR)/string/rules.mk

include make/module.mk
