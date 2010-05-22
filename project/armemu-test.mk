# top level project rules for the armemu-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := armemu
MODULES += \
	lib/bio \
	lib/partition \
	lib/bcache \
	lib/fs \
	lib/fs/ext2 \
	lib/gfx \
	lib/gfxconsole \
	lib/text \
	lib/tga \
	app/tests \
	app/shell

# extra rules to copy the armemu.conf file to the build dir
#$(BUILDDIR)/armemu.conf: $(LOCAL_DIR)/armemu.conf
#	@echo copy $< to $@
#	$(NOECHO)cp $< $@

#EXTRA_BUILDDEPS += $(BUILDDIR)/armemu.conf
#GENERATED += $(BUILDDIR)/armemu.conf
