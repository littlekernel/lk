LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/generic_string.c \
	$(LOCAL_DIR)/string_tests.c \

MODULE_DEPS += \
	lib/version \

# generic_string.c compiles the libc C sources, so give them libc's flags
MODULE_COMPILEFLAGS += -fno-builtin

include make/module.mk
