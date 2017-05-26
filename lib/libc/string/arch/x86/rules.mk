LOCAL_DIR := $(GET_LOCAL_DIR)

ifeq ($(SUBARCH), x86-64)
include $(LOCAL_DIR)/64/rules.mk
else
include $(LOCAL_DIR)/32/rules.mk
endif

LOCAL_DIR :=
