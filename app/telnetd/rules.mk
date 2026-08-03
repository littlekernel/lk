LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/telnetd.c \

MODULE_DEPS := \
	lib/console \
	lib/minip \

# A telnet session is only a real second shell if commands, which print with
# plain printf, follow the session's stdout binding.
WITH_THREAD_STDOUT := 1

include make/module.mk
