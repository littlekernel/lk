LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/loader.c \

# disable a few warnings that some of this module's code trips over.
# -Wstringop-overflow is a gcc only option, and clang rejects an unknown -Wno-
# form outright (-Wunknown-warning-option, fatal under WERROR=1), so probe for
# the positive form first. -Warray-bounds exists in both and needs no guard.
MODULE_COMPILEFLAGS += -Wno-array-bounds
ifeq ($(call is_warning_flag_supported,-Wstringop-overflow),yes)
MODULE_COMPILEFLAGS += -Wno-stringop-overflow
endif

MODULE_DEPS := \
    lib/cksum \
    lib/console \
    lib/tftp  \
    lib/elf

include make/module.mk
