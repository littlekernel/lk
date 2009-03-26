# top level project rules for the x86-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := x86
#DEFINES += \
#	USE_UNDERSCORE=1
MODULES += \
	app/tests \
	app/shell

# extra rules to copy the x86.conf file to the build dir
#$(BUILDDIR)/x86.conf: $(LOCAL_DIR)/x86.conf
#	@echo copy $< to $@
#	$(NOECHO)cp $< $@

#EXTRA_BUILDDEPS += $(BUILDDIR)/x86.conf
#GENERATED += $(BUILDDIR)/x86.conf
