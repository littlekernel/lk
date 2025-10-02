LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/unittest.c \
	$(LOCAL_DIR)/all_tests.c \

ifeq (true,$(call TOBOOL,$(RUN_UNITTESTS_AT_BOOT)))
$(info Boot unit tests enabled)
MODULE_DEFINES += RUN_UNITTESTS_AT_BOOT=1
endif

include make/module.mk
