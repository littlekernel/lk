LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/disktest.c \
	$(LOCAL_DIR)/disktest_lk_backend.c

MODULE_DEPS += \
	lib/bio

MODULE_OPTIONS := extra_warnings

MODULE_DEFINES += \
	DISKTEST_BACKEND_LK=1

include make/module.mk
