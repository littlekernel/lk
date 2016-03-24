LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	platform \
	target \
	app \
	dev \
	kernel

MODULE_SRCS := \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/main.c \

EXTRA_LINKER_SCRIPTS += $(LOCAL_DIR)/init.ld

include make/module.mk
