LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/otp.c \
	$(LOCAL_DIR)/otp_asm.S \


include make/module.mk
