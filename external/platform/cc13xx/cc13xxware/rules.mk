LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/driverlib/setup.c \
	$(LOCAL_DIR)/driverlib/cpu.c \
	$(LOCAL_DIR)/driverlib/osc.c \
	$(LOCAL_DIR)/driverlib/chipinfo.c \
	$(LOCAL_DIR)/driverlib/rfc.c

# ideally includes would be under $(LOCAL_DIR)/include
# so this would not be needed...
GLOBAL_INCLUDES += $(LOCAL_DIR)

include make/module.mk
