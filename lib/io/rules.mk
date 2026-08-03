LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

CONSOLE_OUTPUT_TO_PLATFORM_PUTC ?= 1

MODULE_DEPS := \
	lib/cbuf

MODULE_DEFINES += \
	CONSOLE_OUTPUT_TO_PLATFORM_PUTC=$(CONSOLE_OUTPUT_TO_PLATFORM_PUTC)

# NOTE: CONSOLE_SERIALIZE_OUTPUT and CONSOLE_RING_SIZE are resolved globally in
# engine.mk rather than here, because they describe behavior other modules can
# observe (lib/io/test asserts against them) and so must not be module scoped.

MODULE_SRCS += \
   $(LOCAL_DIR)/console.c \
   $(LOCAL_DIR)/io.c \

MODULE_OPTIONS := test

include make/module.mk
