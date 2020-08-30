LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/drivers/src/nrfx_usbd.c \
	$(LOCAL_DIR)/mdk/system_nrf52.c \

#	$(LOCAL_DIR)/driverlib/cpu.c \
#	$(LOCAL_DIR)/driverlib/osc.c \
#	$(LOCAL_DIR)/driverlib/chipinfo.c \
#	$(LOCAL_DIR)/driverlib/rfc.c

# ideally includes would be under $(LOCAL_DIR)/include
# so this would not be needed...
GLOBAL_INCLUDES += $(LOCAL_DIR) \
	$(LOCAL_DIR)/drivers/include \
	$(LOCAL_DIR)/templates \
	$(LOCAL_DIR)/mdk \

include make/module.mk
