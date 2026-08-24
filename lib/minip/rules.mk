LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/iovec \
	lib/libcpp \
	lib/pool

MODULE_SRCS += \
	$(LOCAL_DIR)/arp.cpp \
	$(LOCAL_DIR)/chksum.c \
	$(LOCAL_DIR)/dhcp.cpp \
	$(LOCAL_DIR)/dns.cpp \
	$(LOCAL_DIR)/lk_console.c \
	$(LOCAL_DIR)/minip.cpp \
	$(LOCAL_DIR)/netif.cpp \
	$(LOCAL_DIR)/stack.cpp \
	$(LOCAL_DIR)/pktbuf.cpp \
	$(LOCAL_DIR)/tcp.cpp \
	$(LOCAL_DIR)/udp.c

MODULE_OPTIONS := test

include make/module.mk
