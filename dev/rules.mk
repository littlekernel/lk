LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/dev.c \
	$(LOCAL_DIR)/driver.c \
	$(LOCAL_DIR)/class/block_api.c \
	$(LOCAL_DIR)/class/i2c_api.c \
	$(LOCAL_DIR)/class/spi_api.c \
	$(LOCAL_DIR)/class/uart_api.c \
	$(LOCAL_DIR)/class/fb_api.c \
	$(LOCAL_DIR)/class/netif_api.c \

include make/module.mk
