ifndef ARCH_arm64_TOOLCHAIN_INCLUDED
ARCH_arm64_TOOLCHAIN_INCLUDED := 1

# if TOOLCHAIN_PREFIX is not empty, try to use it first
ifneq ($(TOOLCHAIN_PREFIX),)
ARCH_arm64_TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREFIX)
FOUNDTOOL=$(shell which $(ARCH_arm64_TOOLCHAIN_PREFIX)gcc)
endif

ifndef ARCH_arm64_TOOLCHAIN_PREFIX
ARCH_arm64_TOOLCHAIN_PREFIX := aarch64-elf-
FOUNDTOOL=$(shell which $(ARCH_arm64_TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
ARCH_arm64_TOOLCHAIN_PREFIX := aarch64-linux-android-
FOUNDTOOL=$(shell which $(ARCH_arm64_TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
$(warning cannot find toolchain in path, assuming aarch64-elf- prefix)
ARCH_arm64_TOOLCHAIN_PREFIX := aarch64-elf-
endif
endif
endif
endif
