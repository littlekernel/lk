LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/bios32.cpp \
	$(LOCAL_DIR)/debug.cpp \
	$(LOCAL_DIR)/pci.cpp \
	$(LOCAL_DIR)/type1.cpp \

include make/module.mk
