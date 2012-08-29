LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/libc \
	lib/debug \
	lib/heap

MODULE_OBJS := \
	debug.o \
	dpc.o \
	event.o \
	main.o \
	mutex.o \
	thread.o \
	timer.o

include make/module.mk
