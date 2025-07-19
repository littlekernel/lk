LOCAL_MAKEFILE:=$(MAKEFILE_LIST)

# macros used all over the build system
include make/macros.mk

BUILDROOT ?= .

# 'make spotless' is a special rule that skips most of the rest of the build system and
# simply deletes everything in build-*
ifeq ($(MAKECMDGOALS),spotless)
spotless:
	rm -rf -- "$(BUILDROOT)"/build-*
else

ifndef LKROOT
$(error please define LKROOT to the root of the lk build system)
endif

# any local environment overrides can optionally be placed in local.mk
-include local.mk

# If one of our goals (from the commandline) happens to have a
# matching project/goal.mk, then we should re-invoke make with
# that project name specified...

project-name := $(firstword $(MAKECMDGOALS))

ifneq ($(project-name),)
ifneq ($(strip $(foreach d,$(LKINC),$(wildcard $(d)/project/$(project-name).mk))),)
do-nothing := 1
$(MAKECMDGOALS) _all: make-make
	@:
make-make:
	@PROJECT=$(project-name) $(MAKE) -rR -f $(LOCAL_MAKEFILE) $(filter-out $(project-name), $(MAKECMDGOALS))

.PHONY: make-make
endif # expansion of project-name
endif # project-name != null

# some additional rules to print some help
include make/help.mk

ifeq ($(do-nothing),)

ifeq ($(PROJECT),)

ifneq ($(DEFAULT_PROJECT),)
PROJECT := $(DEFAULT_PROJECT)
else
$(error No project specified. Use 'make list' for a list of projects or 'make help' for additional help)
endif # DEFAULT_PROJECT == something

endif # PROJECT == null

DEBUG ?= 2

BUILDDIR_SUFFIX ?=
BUILDDIR := $(BUILDROOT)/build-$(PROJECT)$(BUILDDIR_SUFFIX)
OUTBIN := $(BUILDDIR)/lk.bin
OUTELF := $(BUILDDIR)/lk.elf
CONFIGHEADER := $(BUILDDIR)/config.h

GLOBAL_INCLUDES := $(BUILDDIR) $(addsuffix /include,$(LKINC))
GLOBAL_OPTFLAGS ?= $(ARCH_OPTFLAGS)
GLOBAL_COMPILEFLAGS := -g -include $(CONFIGHEADER)
GLOBAL_COMPILEFLAGS += -Wextra -Wall -Werror=return-type -Wshadow -Wdouble-promotion
GLOBAL_COMPILEFLAGS += -Wno-multichar -Wno-unused-parameter -Wno-unused-function -Wno-unused-label
GLOBAL_COMPILEFLAGS += -fno-common
# Build with -ffreestanding since we are building an OS kernel and cannot
# rely on all hosted environment functionality being present.
GLOBAL_COMPILEFLAGS += -ffreestanding
GLOBAL_CFLAGS := --std=gnu11 -Werror-implicit-function-declaration -Wstrict-prototypes -Wwrite-strings
GLOBAL_CPPFLAGS := --std=c++14 -fno-exceptions -fno-rtti -fno-threadsafe-statics
GLOBAL_ASMFLAGS := -DASSEMBLY
GLOBAL_LDFLAGS :=

ifeq ($(UBSAN), 1)
# Inject lib/ubsan directly into MODULE_DEPS
# lib/ubsan will itself add the needed CFLAGS
MODULE_DEPS += lib/ubsan
endif

# flags that are sometimes nice to enable to catch problems but too strict to have on all the time.
# add to global flags from time to time to find things, otherwise only available with a module
# option (see make/module.mk re: MODULE_OPTIONS).
EXTRA_MODULE_COMPILEFLAGS := -Wmissing-declarations -Wredundant-decls
EXTRA_MODULE_CFLAGS := -Wmissing-prototypes
EXTRA_MODULE_CPPFLAGS :=
EXTRA_MODULE_ASMFLAGS :=

#GLOBAL_COMPILEFLAGS += -Wpacked
#GLOBAL_COMPILEFLAGS += -Wpadded
#GLOBAL_COMPILEFLAGS += -Winline
#GLOBAL_COMPILEFLAGS += -Wredundant-decls

# if WERROR is set, add to the compile args
ifeq (true,$(call TOBOOL,$(WERROR)))
GLOBAL_COMPILEFLAGS += -Werror
endif

GLOBAL_LDFLAGS += $(addprefix -L,$(LKINC))

# Architecture specific compile flags
ARCH_COMPILEFLAGS :=
ARCH_COMPILEFLAGS_NOFLOAT := # flags used when compiling with floating point support
ARCH_COMPILEFLAGS_FLOAT := # flags for when not compiling with floating point support
ARCH_CFLAGS :=
ARCH_CPPFLAGS :=
ARCH_ASMFLAGS :=
ARCH_LDFLAGS :=
ARCH_OBJDUMP_FLAGS :=
THUMBCFLAGS := # optional compile switches set by arm architecture when compiling in thumb mode

# top level rule
all:: $(OUTBIN) $(OUTELF).lst $(OUTELF).debug.lst $(OUTELF).sym $(OUTELF).sym.sorted $(OUTELF).size $(OUTELF).dump $(BUILDDIR)/srcfiles.txt $(BUILDDIR)/include_paths.txt

# master module object list
ALLMODULE_OBJS :=

# master object list (for dep generation)
ALLOBJS :=

# master source file list
ALLSRCS :=

# a linker script needs to be declared in one of the project/target/platform files
LINKER_SCRIPT :=

# anything you add here will be deleted in make clean
GENERATED := $(CONFIGHEADER)

# anything added to GLOBAL_DEFINES will be put into $(BUILDDIR)/config.h
GLOBAL_DEFINES := LK=1

# Anything added to GLOBAL_SRCDEPS will become a dependency of every source file in the system.
# Useful for header files that may be included by one or more source files.
GLOBAL_SRCDEPS := $(CONFIGHEADER)

# these need to be filled out by the project/target/platform rules.mk files
TARGET :=
PLATFORM :=
ARCH :=
ALLMODULES :=

# add any external module dependencies
MODULES := $(EXTERNAL_MODULES)

# any .mk specified here will be included before build.mk
EXTRA_BUILDRULES :=

# any rules you put here will also be built by the system before considered being complete
EXTRA_BUILDDEPS :=

# any rules you put here will be depended on in clean builds
EXTRA_CLEANDEPS :=

# any objects you put here get linked with the final image
EXTRA_OBJS :=

# any extra linker scripts to be put on the command line
EXTRA_LINKER_SCRIPTS :=

# if someone defines this, the build id will be pulled into lib/version
BUILDID ?=

# comment out or override if you want to see the full output of each command
NOECHO ?= @

# Any modules you want to explictly prevent from being used
DENY_MODULES :=

# try to include the project file
-include project/$(PROJECT).mk
ifndef TARGET
$(error couldn't find project or project doesn't define target)
endif
include target/$(TARGET)/rules.mk
ifndef PLATFORM
$(error couldn't find target or target doesn't define platform)
endif
include platform/$(PLATFORM)/rules.mk

ifndef ARCH
$(error couldn't find arch or platform doesn't define arch)
endif

# list the architecture specified in the project/target/platform rules.mk and early terminate.
ifeq ($(MAKECMDGOALS), list-arch)
$(info ARCH = $(ARCH))
.PHONY: list-arch
list-arch:
else

include arch/$(ARCH)/rules.mk
ifndef TOOLCHAIN_PREFIX
$(error TOOLCHAIN_PREFIX not set in the arch rules.mk)
endif

ifeq ($(MAKECMDGOALS), list-toolchain)
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))
.PHONY: list-toolchain
list-toolchain:
else

# default to no ccache
CCACHE ?=
CC ?= $(CCACHE) $(TOOLCHAIN_PREFIX)gcc
LD ?= $(TOOLCHAIN_PREFIX)ld
OBJDUMP ?= $(TOOLCHAIN_PREFIX)objdump
OBJCOPY ?= $(TOOLCHAIN_PREFIX)objcopy
CPPFILT ?= $(TOOLCHAIN_PREFIX)c++filt
SIZE ?= $(TOOLCHAIN_PREFIX)size
NM ?= $(TOOLCHAIN_PREFIX)nm
STRIP ?= $(TOOLCHAIN_PREFIX)strip

# Detect whether we are using ld.lld. If we don't detect ld.lld, we assume
# it's ld.bfd. This check can be refined in the future if we need to handle
# more cases (e.g. ld.gold).
LINKER_TYPE := $(shell $(LD) -v 2>&1 | grep -q "LLD" && echo lld || echo bfd)
$(info LINKER_TYPE=$(LINKER_TYPE))
# Detect whether we are compiling with GCC or Clang
COMPILER_TYPE := $(shell $(CC) -v 2>&1 | grep -q "clang version" && echo clang || echo gcc)
$(info COMPILER_TYPE=$(COMPILER_TYPE))

# Now that CC is defined we can check if warning flags are supported and add
# them to GLOBAL_COMPILEFLAGS if they are.
ifeq ($(call is_warning_flag_supported,-Wnonnull-compare),yes)
GLOBAL_COMPILEFLAGS += -Wno-nonnull-compare
endif
# Ideally we would move this check to arm64/rules.mk, but we can only check
# for supported warning flags once CC is defined.
ifeq ($(ARCH),arm64)
# Clang incorrectly diagnoses msr operations as need a 64-bit operand even if
# the underlying register is actually 32 bits. Silence this common warning.
ifeq ($(call is_warning_flag_supported,-Wasm-operand-widths),yes)
ARCH_COMPILEFLAGS += -Wno-asm-operand-widths
endif
endif

ifeq ($(ARCH),riscv)
# ld.lld does not support linker relaxations yet.
# TODO: This is no longer true as of LLVM 15, so should add a version check
ifeq ($(LINKER_TYPE),lld)
ARCH_COMPILEFLAGS += -mno-relax
# Work around out-of-range undef-weak relocations when building with clang and
# linking with ld.lld. This is not a problem with ld.bfd since ld.bfd rewrites
# the instructions to avoid the out-of-range PC-relative relocation
# See https://github.com/riscv-non-isa/riscv-elf-psabi-doc/issues/126 for more
# details. For now, the simplest workaround is to build with -fpie when using
# a version of clang that does not include https://reviews.llvm.org/D107280.
# TODO: Add a clang 17 version check now that the review has been merged.
ifeq ($(COMPILER_TYPE),clang)
# We also add the -fdirect-access-external-data flag is added to avoid the
# majority of the performance overhead caused by -fPIE.
ARCH_COMPILEFLAGS += -fPIE -fdirect-access-external-data
endif
endif
endif

$(info PROJECT = $(PROJECT))
$(info PLATFORM = $(PLATFORM))
$(info TARGET = $(TARGET))
$(info ARCH = $(ARCH))
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))
$(info DEBUG = $(DEBUG))

# include the top level module that includes basic always-there modules
include top/rules.mk

# recursively include any modules in the MODULE variable, leaving a trail of included
# modules in the ALLMODULES list
include make/recurse.mk

# add some automatic configuration defines
GLOBAL_DEFINES += \
	PROJECT_$(PROJECT)=1 \
	PROJECT=\"$(PROJECT)\" \
	TARGET_$(TARGET)=1 \
	TARGET=\"$(TARGET)\" \
	PLATFORM_$(PLATFORM)=1 \
	PLATFORM=\"$(PLATFORM)\" \
	ARCH_$(ARCH)=1 \
	ARCH=\"$(ARCH)\" \
	$(addsuffix =1,$(addprefix WITH_,$(ALLMODULES)))

# debug build?
ifneq ($(DEBUG),)
GLOBAL_DEFINES += \
	LK_DEBUGLEVEL=$(DEBUG)
endif

# allow additional defines from outside the build system
ifneq ($(EXTERNAL_DEFINES),)
GLOBAL_DEFINES += $(EXTERNAL_DEFINES)
$(info EXTERNAL_DEFINES = $(EXTERNAL_DEFINES))
endif

# prefix all of the paths in GLOBAL_INCLUDES with -I
GLOBAL_INCLUDES := $(addprefix -I,$(GLOBAL_INCLUDES))

# test for some old variables
ifneq ($(INCLUDES),)
$(error INCLUDES variable set, please move to GLOBAL_INCLUDES: $(INCLUDES))
endif
ifneq ($(DEFINES),)
$(error DEFINES variable set, please move to GLOBAL_DEFINES: $(DEFINES))
endif

# try to have the compiler output colorized error messages if available
export GCC_COLORS ?= 1

# the logic to compile and link stuff is in here
include make/build.mk

DEPS := $(ALLOBJS:%o=%d)

# put all of the global build flags in config.h to force a rebuild if any change
GLOBAL_DEFINES += GLOBAL_INCLUDES=\"$(subst $(SPACE),_,$(GLOBAL_INCLUDES))\"
GLOBAL_DEFINES += GLOBAL_COMPILEFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_COMPILEFLAGS))\"
GLOBAL_DEFINES += GLOBAL_OPTFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_OPTFLAGS))\"
GLOBAL_DEFINES += GLOBAL_CFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_CFLAGS))\"
GLOBAL_DEFINES += GLOBAL_CPPFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_CPPFLAGS))\"
GLOBAL_DEFINES += GLOBAL_ASMFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_ASMFLAGS))\"
GLOBAL_DEFINES += GLOBAL_LDFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_LDFLAGS))\"
GLOBAL_DEFINES += ARCH_COMPILEFLAGS=\"$(subst $(SPACE),_,$(ARCH_COMPILEFLAGS))\"
GLOBAL_DEFINES += ARCH_COMPILEFLAGS_FLOAT=\"$(subst $(SPACE),_,$(ARCH_COMPILEFLAGS_FLOAT))\"
GLOBAL_DEFINES += ARCH_COMPILEFLAGS_NOFLOAT=\"$(subst $(SPACE),_,$(ARCH_COMPILEFLAGS_NOFLOAT))\"
GLOBAL_DEFINES += ARCH_CFLAGS=\"$(subst $(SPACE),_,$(ARCH_CFLAGS))\"
GLOBAL_DEFINES += ARCH_CPPFLAGS=\"$(subst $(SPACE),_,$(ARCH_CPPFLAGS))\"
GLOBAL_DEFINES += ARCH_ASMFLAGS=\"$(subst $(SPACE),_,$(ARCH_ASMFLAGS))\"
GLOBAL_DEFINES += ARCH_LDFLAGS=\"$(subst $(SPACE),_,$(ARCH_LDFLAGS))\"
GLOBAL_DEFINES += TOOLCHAIN_PREFIX=\"$(subst $(SPACE),_,$(TOOLCHAIN_PREFIX))\"

ifneq ($(OBJS),)
$(warning OBJS=$(OBJS))
$(error OBJS is not empty, please convert to new module format)
endif
ifneq ($(OPTFLAGS),)
$(warning OPTFLAGS=$(OPTFLAGS))
$(error OPTFLAGS is not empty, please use GLOBAL_OPTFLAGS or MODULE_OPTFLAGS)
endif
ifneq ($(CFLAGS),)
$(warning CFLAGS=$(CFLAGS))
$(error CFLAGS is not empty, please use GLOBAL_CFLAGS or MODULE_CFLAGS)
endif
ifneq ($(CPPFLAGS),)
$(warning CPPFLAGS=$(CPPFLAGS))
$(error CPPFLAGS is not empty, please use GLOBAL_CPPFLAGS or MODULE_CPPFLAGS)
endif

$(info LIBGCC = $(LIBGCC))
$(info GLOBAL_COMPILEFLAGS = $(GLOBAL_COMPILEFLAGS))
$(info GLOBAL_OPTFLAGS = $(GLOBAL_OPTFLAGS))
$(info ARCH_COMPILEFLAGS = $(ARCH_COMPILEFLAGS))
$(info ARCH_COMPILEFLAGS_FLOAT = $(ARCH_COMPILEFLAGS_FLOAT))
$(info ARCH_COMPILEFLAGS_NOFLOAT = $(ARCH_COMPILEFLAGS_NOFLOAT))

# make all object files depend on any targets in GLOBAL_SRCDEPS
$(ALLOBJS): $(GLOBAL_SRCDEPS)

# any extra top level build dependencies that someone declared.
# build.mk may add to EXTRA_BUILDDEPS, this must be evalauted after build.mk.
all:: $(EXTRA_BUILDDEPS)

clean: $(EXTRA_CLEANDEPS)
	rm -f $(ALLOBJS) $(DEPS) $(GENERATED) $(OUTBIN) $(OUTELF) $(OUTELF).lst $(OUTELF).debug.lst $(OUTELF).sym $(OUTELF).sym.sorted $(OUTELF).size $(OUTELF).hex $(OUTELF).dump

install: all
	scp $(OUTBIN) 192.168.0.4:/tftpboot

tags: $(BUILDDIR)/srcfiles.txt $(BUILDDIR)/include_paths.txt
	$(info generating tags)
	@ctags -L $<

.PHONY: all clean install list-arch list-toolchain tags

# generate a config.h file with all of the GLOBAL_DEFINES laid out in #define format
configheader:

$(CONFIGHEADER): configheader
	@$(call MAKECONFIGHEADER,$@,GLOBAL_DEFINES)

.PHONY: configheader

# Empty rule for the .d files. The above rules will build .d files as a side
# effect. Only works on gcc 3.x and above, however.
%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif

endif # ifeq ($(filter $(MAKECMDGOALS), list-toolchain))

endif # ifeq ($(filter $(MAKECMDGOALS), list-arch))

endif # do-nothing = 1

endif # make spotless
