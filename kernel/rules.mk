LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/libc \
	lib/debug \
	lib/heap

MODULE_SRCS := \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/event.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/mutex.c \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/semaphore.c \
	

include make/module.mk
