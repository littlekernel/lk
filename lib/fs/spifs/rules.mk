LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/spifs.c \

MODULE_DEPS += \
	lib/fs \
	lib/cksum \
	lib/bio

include make/module.mk
