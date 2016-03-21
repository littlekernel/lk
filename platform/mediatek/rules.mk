LOCAL_DIR := $(GET_LOCAL_DIR)

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld

include $(LOCAL_DIR)/$(SUB_PLATFORM)/rules.mk

