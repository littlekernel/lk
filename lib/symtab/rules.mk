LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_OPTIONS += extra_warnings test

MODULE_SRCS += \
	$(LOCAL_DIR)/symtab.c

MODULE_DEPS += \
	lib/libc

MODULE_WEAK_DEPS += \
	lib/console

# The table the lookup code searches is generated from the symbols of a first
# link pass and injected into the final one. Those rules need the complete
# object list, so they live in a file build.mk includes once every module has
# been processed.
SYMTAB_LOCAL_DIR := $(LOCAL_DIR)
EXTRA_BUILDRULES += $(LOCAL_DIR)/symtab_buildrules.mk
EXTRA_OBJS += $(BUILDDIR)/symtab_data.o

include make/module.mk
