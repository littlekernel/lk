LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/mmu.cpp \
	$(LOCAL_DIR)/mmu_contract.cpp \

# The user mode tests take over the arch's user exception hooks for the whole
# image, so a project that hosts its own user space sets this to 0 to leave
# them out and keep the rest of the arch tests.
WITH_ARCH_USPACE_TESTS ?= 1
ifeq ($(call TOBOOL,$(WITH_ARCH_USPACE_TESTS)),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/uspace.cpp \
	$(LOCAL_DIR)/uspace_stubs.S \

endif

# The floating point tests are compiled with floating point support. They are
# guarded internally by #if !WITH_NO_FP (and #if ARM_WITH_VFP for the assembly),
# which is the only way to test it: WITH_NO_FP is a GLOBAL_DEFINES value that
# only exists in config.h, so it cannot be checked from a rules.mk file.
MODULE_FLOAT_SRCS := \
	$(LOCAL_DIR)/float_tests.c \
	$(LOCAL_DIR)/float_instructions.S \

# Keep the floating point arithmetic reproducible across architectures. Without
# this the compiler may contract multiply-add pairs into fma, which changes the
# low bits of the accumulated results the test compares against.
MODULE_COMPILEFLAGS += -ffp-contract=off

MODULE_DEPS := lib/libcpp
MODULE_DEPS += lib/lktl
MODULE_DEPS += lib/unittest

include make/module.mk

