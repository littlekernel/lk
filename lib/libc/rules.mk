LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/atoi.o \
	$(LOCAL_DIR)/ctype.o \
	$(LOCAL_DIR)/printf.o \
	$(LOCAL_DIR)/malloc.o \
	$(LOCAL_DIR)/rand.o \
	$(LOCAL_DIR)/eabi.o


include $(LOCAL_DIR)/string/rules.mk

ifeq ($(WITH_CPP_SUPPORT),true)
OBJS += \
	$(LOCAL_DIR)/new.o \
	$(LOCAL_DIR)/atexit.o \
	$(LOCAL_DIR)/pure_virtual.o
endif

