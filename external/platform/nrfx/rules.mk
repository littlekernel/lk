LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/drivers/src/nrfx_clock.c \
	$(LOCAL_DIR)/drivers/src/nrfx_twi.c \
	$(LOCAL_DIR)/drivers/src/nrfx_twi_twim.c \
	$(LOCAL_DIR)/drivers/src/nrfx_twim.c \
	$(LOCAL_DIR)/drivers/src/nrfx_usbd.c \
	$(LOCAL_DIR)/mdk/system_nrf52840.c \
	$(LOCAL_DIR)/soc/nrfx_atomic.c \

# The nrfx library doesn't follow the typical lk include directory layout
#  so adding all dirs that have the needed headers.
GLOBAL_INCLUDES += $(LOCAL_DIR) \
	$(LOCAL_DIR)/drivers/include \
	$(LOCAL_DIR)/templates \
	$(LOCAL_DIR)/mdk \
	$(LOCAL_DIR)/soc \

include make/module.mk
