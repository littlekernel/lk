LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/pktbuf.c \
	$(LOCAL_DIR)/arp.c \
	$(LOCAL_DIR)/chksum.c \
	$(LOCAL_DIR)/minip.c \
	$(LOCAL_DIR)/dhcp.c \
	$(LOCAL_DIR)/lk_console.c

include make/module.mk
