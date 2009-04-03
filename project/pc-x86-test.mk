# top level project rules for the pc-x86-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := pc-x86
MODULES += \
	app/tests \
	app/shell \
	app/pcitests

# extra rules to copy the pc-x86.conf file to the build dir
#$(BUILDDIR)/pc-x86.conf: $(LOCAL_DIR)/pc-x86.conf
#	@echo copy $< to $@
#	$(NOECHO)cp $< $@

#EXTRA_BUILDDEPS += $(BUILDDIR)/pc-x86.conf
#GENERATED += $(BUILDDIR)/pc-x86.conf
