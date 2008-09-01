LOCAL_DIR := $(GET_LOCAL_DIR)

LIBS += debug heap

KOBJS += \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/dpc.o \
	$(LOCAL_DIR)/event.o \
	$(LOCAL_DIR)/main.o \
	$(LOCAL_DIR)/mutex.o \
	$(LOCAL_DIR)/thread.o \
	$(LOCAL_DIR)/timer.o

