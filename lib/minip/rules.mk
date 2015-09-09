LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/cbuf \
	lib/iovec \
	lib/pool

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/arp.c \
	$(LOCAL_DIR)/chksum.c \
	$(LOCAL_DIR)/dhcp.c \
	$(LOCAL_DIR)/lk_console.c \
	$(LOCAL_DIR)/minip.c \
	$(LOCAL_DIR)/net_timer.c \
	$(LOCAL_DIR)/pktbuf.c \
	$(LOCAL_DIR)/tcp.c \
	$(LOCAL_DIR)/udp.c

include make/module.mk
