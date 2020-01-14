LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

TOOLCHAIN_PREFIX := vc4-elf-

$(BUILDDIR)/system-onesegment.ld: $(LOCAL_DIR)/start.ld
	@echo generating $@
	@$(MKDIR)
	echo TODO: properly template the linker script
	cp $< $@

# TODO, fix the linker flags
ARCH_LDFLAGS += -L/nix/store/cwpy4q0qvdwdif1zfwnfg5gi50c6j9w8-vc4-elf-stage-final-gcc-debug-6.5.0/lib/gcc/vc4-elf/6.2.1/

MODULE_SRCS += \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/intc.c \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/thread_asm.S \
	$(LOCAL_DIR)/interrupt.S \

GLOBAL_DEFINES += PLATFORM_HAS_DYNAMIC_TIMER=1

include make/module.mk
