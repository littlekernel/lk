# top level project rules for the armemu-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := armemu
LIBS += lwip
APPS += httpd

# use dhcp
#DEFINES += \
	WITH_DHCP=1

# use static ip
DEFINES += \
	WITH_STATIC_IP=1 \
	IP_ADDR=0xc0a80202 \
	GW_ADDR=0xc0a80101 \
	NETMASK=0xffffff00

OBJS += \
	$(LOCAL_DIR)/init.o

# extra rules to copy the armemu.conf file to the build dir
$(BUILDDIR)/armemu.conf: $(LOCAL_DIR)/armemu.conf
	@echo copy $< to $@
	$(NOECHO)cp $< $@

EXTRA_BUILDDEPS += $(BUILDDIR)/armemu.conf
GENERATED += $(BUILDDIR)/armemu.conf
