LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULES += \

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include \
	$(LOCAL_DIR)/include/lwip \
	$(LOCAL_DIR)/include/posix \

MODULE_SRCS += \
	$(LOCAL_DIR)/sys_arch.c \
	$(LOCAL_DIR)/cmd.c \
	$(LOCAL_DIR)/netif.c \
	$(LOCAL_DIR)/api/api_lib.c \
	$(LOCAL_DIR)/api/api_msg.c \
	$(LOCAL_DIR)/api/err.c \
	$(LOCAL_DIR)/api/netbuf.c \
	$(LOCAL_DIR)/api/netifapi.c \
	$(LOCAL_DIR)/api/sockets.c \
	$(LOCAL_DIR)/api/tcpip.c \
	$(LOCAL_DIR)/core/def.c \
	$(LOCAL_DIR)/core/dhcp.c \
	$(LOCAL_DIR)/core/dns.c \
	$(LOCAL_DIR)/core/init.c \
	$(LOCAL_DIR)/core/mem.c \
	$(LOCAL_DIR)/core/memp.c \
	$(LOCAL_DIR)/core/netif.c \
	$(LOCAL_DIR)/core/pbuf.c \
	$(LOCAL_DIR)/core/raw.c \
	$(LOCAL_DIR)/core/stats.c \
	$(LOCAL_DIR)/core/sys.c \
	$(LOCAL_DIR)/core/tcp.c \
	$(LOCAL_DIR)/core/tcp_in.c \
	$(LOCAL_DIR)/core/tcp_out.c \
	$(LOCAL_DIR)/core/timers.c \
	$(LOCAL_DIR)/core/udp.c \
	$(LOCAL_DIR)/netif/etharp.c \

LWIP_IP_TYPE := IPV4

ifeq ($(LWIP_IP_TYPE),IPV4)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include/ipv4 \

MODULE_SRCS += \
	$(LOCAL_DIR)/core/ipv4/autoip.c \
	$(LOCAL_DIR)/core/ipv4/icmp.c \
	$(LOCAL_DIR)/core/ipv4/igmp.c \
	$(LOCAL_DIR)/core/ipv4/inet.c \
	$(LOCAL_DIR)/core/ipv4/inet_chksum.c \
	$(LOCAL_DIR)/core/ipv4/ip.c \
	$(LOCAL_DIR)/core/ipv4/ip_addr.c \
	$(LOCAL_DIR)/core/ipv4/ip_frag.c \

endif
	
include make/module.mk

