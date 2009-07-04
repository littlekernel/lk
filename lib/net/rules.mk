LOCAL_DIR := $(GET_LOCAL_DIR)

OBJS += \
	$(LOCAL_DIR)/arp.o \
	$(LOCAL_DIR)/cbuf.o \
	$(LOCAL_DIR)/ethernet.o \
	$(LOCAL_DIR)/hash.o \
	$(LOCAL_DIR)/icmp.o \
	$(LOCAL_DIR)/if.o \
	$(LOCAL_DIR)/ipv4.o \
	$(LOCAL_DIR)/loopback.o \
	$(LOCAL_DIR)/misc.o \
	$(LOCAL_DIR)/net.o \
	$(LOCAL_DIR)/net_timer.o \
	$(LOCAL_DIR)/queue.o \
	$(LOCAL_DIR)/socket.o \
	$(LOCAL_DIR)/tcp.o \
	$(LOCAL_DIR)/udp.o

