LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_OPTIONS += extra_warnings test

MODULE_SRCS += \
	$(LOCAL_DIR)/symtab.c

MODULE_DEPS += \
	lib/libc

MODULE_WEAK_DEPS += \
	lib/console

# A table naming every function costs tens of kilobytes, which is a tenth of
# the flash on the targets LK_EMBEDDED marks. Link the empty table there
# instead: lookups report nothing and a backtrace falls back to bare
# addresses, which can still be resolved on the host against lk.elf.sym.
ifeq (0,$(LK_EMBEDDED))

# The table the lookup code searches is generated from the symbols of a first
# link pass and injected into the final one. Those rules need the complete
# object list, so they live in a file build.mk includes once every module has
# been processed.
SYMTAB_LOCAL_DIR := $(LOCAL_DIR)
EXTRA_BUILDRULES += $(LOCAL_DIR)/symtab_buildrules.mk
EXTRA_OBJS += $(BUILDDIR)/symtab_data.o
GLOBAL_DEFINES += SYMTAB_HAVE_TABLE=1

else

MODULE_SRCS += $(LOCAL_DIR)/symtab_placeholder.c
GLOBAL_DEFINES += SYMTAB_HAVE_TABLE=0

endif

include make/module.mk
