LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/arp_tests.c \
	$(LOCAL_DIR)/chksum_tests.c \
	$(LOCAL_DIR)/dhcp_tests.c \
	$(LOCAL_DIR)/dns_tests.c \
	$(LOCAL_DIR)/pktbuf_tests.c \
	$(LOCAL_DIR)/stack_tests.c \
	$(LOCAL_DIR)/tcp_tests.c \
	$(LOCAL_DIR)/testnetif.c \
	$(LOCAL_DIR)/udp_tests.c

MODULE_DEPS += lib/minip
MODULE_DEPS += lib/unittest

include make/module.mk
