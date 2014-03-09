LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/string_tests.c \

ifeq ($(ARCH),arm)
ifeq ($(SUBARCH),arm)
MODULE_SRCS += \
	$(LOCAL_DIR)/arm/mymemcpy.S \
	$(LOCAL_DIR)/arm/mymemset.S
endif
ifeq ($(SUBARCH),arm-m)
MODULE_SRCS += \
	$(LOCAL_DIR)/arm-m/mymemcpy.S \
	$(LOCAL_DIR)/arm-m/mymemset.S
endif
endif

include make/module.mk
