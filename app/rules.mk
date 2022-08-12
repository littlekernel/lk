LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/app.c \
	$(LOCAL_DIR)/ksj.c \
	$(LOCAL_DIR)/nomutex.c \
	$(LOCAL_DIR)/mutex_sync.c

include make/module.mk
