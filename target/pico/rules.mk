LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

PLATFORM := rp20xx

GLOBAL_DEFINES += \
	TARGET_HAS_DEBUG_LED=1

MODULE_SRCS += \
	$(LOCAL_DIR)/boot.stage2.S \
	$(LOCAL_DIR)/target.c

include make/module.mk
