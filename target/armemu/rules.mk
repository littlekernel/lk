# mostly null target configuration for the arm emulator, since there's only one real
# implementation.
LOCAL_DIR := $(GET_LOCAL_DIR)

PLATFORM := armemu

$(BUILDDIR)/armemu.conf: $(LOCAL_DIR)/armemu.conf
	cp $< $@

EXTRA_BUILDDEPS += $(BUILDDIR)/armemu.conf
GENERATED += $(BUILDDIR)/armemu.conf
