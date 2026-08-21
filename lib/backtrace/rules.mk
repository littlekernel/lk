LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_OPTIONS += extra_warnings test

MODULE_DEPS += \
	kernel \
	lib/libc

# The symbol table is what turns the addresses into names, but a backtrace of
# bare addresses is still worth having, and can be symbolized on the host
# against lk.elf.sym. Same for the console command.
MODULE_WEAK_DEPS += \
	lib/console \
	lib/symtab

# Architectures whose frame layout backtrace.c knows how to walk. Elsewhere
# the module still builds, so a project fragment can enable it for every
# target it covers, but reports that it cannot walk the stack.
BACKTRACE_ARCHS := arm64 x86 riscv

# Keeping a frame pointer in every function costs code size across the whole
# image, not just in this module, which the targets LK_EMBEDDED marks cannot
# spare. They get the same do nothing build as an unsupported architecture.
ifeq (0,$(LK_EMBEDDED))
BACKTRACE_ARCH := $(filter $(ARCH),$(BACKTRACE_ARCHS))
endif

ifneq ($(BACKTRACE_ARCH),)

MODULE_SRCS += $(LOCAL_DIR)/backtrace.c

# Walking frame records means there have to be frame records, which the
# compiler otherwise drops at the optimization levels the architectures
# default to. This has to reach every module, not just this one.
GLOBAL_COMPILEFLAGS += -fno-omit-frame-pointer
GLOBAL_DEFINES += BACKTRACE_SUPPORTED=1

else

MODULE_SRCS += $(LOCAL_DIR)/backtrace_stub.c
GLOBAL_DEFINES += BACKTRACE_SUPPORTED=0

endif

include make/module.mk
