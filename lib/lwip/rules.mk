LOCAL_DIR := $(GET_LOCAL_DIR)

DEFINES += WITH_LWIP=1

INCLUDES += \
	-I$(LOCAL_DIR)/include \
	-I$(LOCAL_DIR)/src/include \
	-I$(LOCAL_DIR)/src/include/ipv4

LWIP_CORE := \
	dhcp.o \
	inet.o \
	ipv4/icmp.o \
	ipv4/ip.o \
	ipv4/ip_addr.o \
	ipv4/ip_frag.o \
	mem.o \
	memp.o \
	netif.o \
	pbuf.o \
	raw.o \
	stats.o \
	sys.o \
	tcp.o \
	tcp_in.o \
	tcp_out.o \
	udp.o

LWIP_API := \
	api_lib.o \
	api_msg.o \
	err.o \
	sockets.o \
	tcpip.o 

LWIP_NETIF := \
	etharp.o

OBJS += \
	$(LOCAL_DIR)/init.o \
	$(LOCAL_DIR)/sys_arch.o \
	$(addprefix $(LOCAL_DIR)/src/core/,$(LWIP_CORE)) \
	$(addprefix $(LOCAL_DIR)/src/api/,$(LWIP_API)) \
	$(addprefix $(LOCAL_DIR)/src/netif/,$(LWIP_NETIF))
