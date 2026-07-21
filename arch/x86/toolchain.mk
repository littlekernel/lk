ifeq ($(TOOLCHAIN),clang)
CLANG_CC := $(if $(CLANG_BINDIR),$(CLANG_BINDIR)/)clang
FOUNDTOOL=$(shell which $(CLANG_CC) 2>/dev/null)
ifeq ($(SUBARCH),x86-32)
ARCH_x86_TOOLCHAIN_PREFIX ?= i386-elf-
else
ARCH_x86_64_TOOLCHAIN_PREFIX ?= x86_64-elf-
endif
else

# x86-32 toolchain
ifeq ($(SUBARCH),x86-32)
ifndef ARCH_x86_TOOLCHAIN_INCLUDED
ARCH_x86_TOOLCHAIN_INCLUDED := 1

ifndef ARCH_x86_TOOLCHAIN_PREFIX
ARCH_x86_TOOLCHAIN_PREFIX := i386-elf-
endif

FOUNDTOOL=$(shell which $(ARCH_x86_TOOLCHAIN_PREFIX)gcc)

ifeq ($(FOUNDTOOL),)
$(warning cannot find toolchain in path, assuming i386-elf- prefix)
ARCH_x86_TOOLCHAIN_PREFIX := i386-elf-
endif

endif
endif

# x86-64 toolchain
ifeq ($(SUBARCH),x86-64)
ifndef ARCH_x86_64_TOOLCHAIN_INCLUDED
ARCH_x86_64_TOOLCHAIN_INCLUDED := 1

ifndef ARCH_x86_64_TOOLCHAIN_PREFIX
ARCH_x86_64_TOOLCHAIN_PREFIX := x86_64-elf-
endif

FOUNDTOOL=$(shell which $(ARCH_x86_64_TOOLCHAIN_PREFIX)gcc)

ifeq ($(FOUNDTOOL),)
$(warning cannot find toolchain in path, assuming x86_64-elf- prefix)
ARCH_x86_64_TOOLCHAIN_PREFIX := x86_64-elf-
endif

endif
endif

endif # TOOLCHAIN == clang

