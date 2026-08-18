LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_OPTIONS := extra_warnings

# The UEFI boot stub runs at whatever address the firmware loaded the image,
# before the kernel proper is entered, so it must be position independent and
# fully self-contained (no calls into the rest of the kernel, which is built
# -mcmodel=kernel -fno-pic). See the header comment in efi-stub.c.
# These override the arch's -fno-pic -mcmodel=kernel: module flags come after
# arch flags on the compile line.
MODULE_COMPILEFLAGS := -fpie -mcmodel=small -fno-jump-tables -fvisibility=hidden

# UBSAN is likewise incompatible with the stub: instrumentation calls into
# __ubsan_handle_* in the kernel proper and emits absolute relocations for its
# source location records, both of which break a relocatable, self-contained
# stub. The reloc check below catches it, but opt out explicitly so a UBSAN=1
# build works rather than failing at that check.
MODULE_COMPILEFLAGS += -fno-sanitize=undefined

MODULE_SRCS += \
	$(LOCAL_DIR)/efi-entry.S \
	$(LOCAL_DIR)/efi-stub.c

# Fail the build if the C side of the stub picks up absolute relocations in
# allocatable sections -- those resolve to link-time addresses and break when
# the firmware loads the image elsewhere. (efi-entry.S is exempt: its PHYS()
# constants are deliberate link-time absolutes used after the image copy.)
EFISTUB_RELOC_STAMP := $(call TOBUILDDIR,$(LOCAL_DIR)/reloc-check.stamp)
$(EFISTUB_RELOC_STAMP): $(call TOBUILDDIR,$(LOCAL_DIR)/efi-stub.c.o)
	@echo checking $< for absolute relocations
	$(NOECHO)$(OBJDUMP) -r $< | awk 'BEGIN{rc=0} /RELOCATION RECORDS FOR/{sec=$$NF} $$2 ~ /^R_X86_64_(64|32|32S)$$/ && sec !~ /debug|eh_frame/ {print "error: EFI stub absolute relocation in "sec" "$$0; rc=1} END{exit rc}'
	$(NOECHO)touch $@

EXTRA_BUILDDEPS += $(EFISTUB_RELOC_STAMP)
GENERATED += $(EFISTUB_RELOC_STAMP)

include make/module.mk
