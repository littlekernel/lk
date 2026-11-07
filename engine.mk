LOCAL_MAKEFILE:=$(MAKEFILE_LIST)

# macros used all over the build system
include make/macros.mk

BUILDROOT ?= .

# Directory holding build-*. An empty BUILDROOT means '.', as in the scripts; taken
# literally it would put build-* at / and spotless would delete it there. Recursive so
# a BUILDROOT set later in local.mk still applies.
BUILDROOT_DIR = $(or $(strip $(BUILDROOT)),.)

# 'make spotless' is a special rule that skips most of the rest of the build system and
# simply deletes everything in build-*
ifeq ($(MAKECMDGOALS),spotless)
spotless:
	rm -rf -- "$(BUILDROOT_DIR)"/build-*
else

ifndef LKROOT
$(error please define LKROOT to the root of the lk build system)
endif

# any local environment overrides can optionally be placed in local.mk.
# Set IGNORE_LOCAL_MK=1 to skip it, so that a build depends only on the
# arguments it was given. Useful when reproducing a CI result or bisecting a
# build break, where a silently included local.mk is easy to overlook. Note
# local.mk is also where a toolchain prefix may be set, so a build that relies
# on one will need it passed explicitly instead.
ifneq ($(IGNORE_LOCAL_MK),1)
-include local.mk
endif

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

# LK_DEBUG is the build's debug switch, 0-3, default 2. It may come from the environment.
# Code sees it as LK_DEBUGLEVEL. DEBUG is never read: the name is often exported for
# unrelated tools (DEBUG=* for Node, DEBUG=1 for TF-A).
$(call CHECK_RENAMED,DEBUG:LK_DEBUG)
ifneq ($(filter environment%,$(origin DEBUG)),)
ifeq ($(origin LK_DEBUG),undefined)
$(warning DEBUG=$(DEBUG) from the environment is ignored, the debug level is set with LK_DEBUG)
endif
endif
LK_DEBUG ?= 2
ifneq ($(words $(LK_DEBUG))$(filter 0 1 2 3,$(LK_DEBUG)),1$(strip $(LK_DEBUG)))
$(error LK_DEBUG must be one of 0 1 2 3, not '$(LK_DEBUG)')
endif

BUILDDIR_SUFFIX ?=
BUILDDIR := $(BUILDROOT_DIR)/build-$(PROJECT)$(BUILDDIR_SUFFIX)

# Source roots are appended to BUILDDIR to form output paths, so none may climb out
# of it. Only normalized copies are checked; the static pattern compile rules keep
# the original spelling.
ESCAPING_ROOTS := $(sort $(foreach r,$(LKROOT) $(LKINC),\
    $(if $(call PATH_WITHIN,$(BUILDDIR),$(BUILDDIR)/$(r)),,$(r))))
ifneq ($(ESCAPING_ROOTS),)
$(error source root(s) '$(ESCAPING_ROOTS)' escape BUILDDIR '$(BUILDDIR)'; \
    symlink the tree into the workspace instead, e.g. ln -s ../lk lk with LKROOT := lk)
endif

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
# -nostdinc++ keeps the toolchain's own C++ standard library headers out of
# the include path; the kernel provides its own freestanding subset in
# lib/libcpp. An accidental #include <vector> should fail loudly instead of
# silently picking up whatever hosted headers sit next to the cross compiler.
GLOBAL_CXXFLAGS := --std=c++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wno-mismatched-tags
GLOBAL_CXXFLAGS += -nostdinc++
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
EXTRA_MODULE_CXXFLAGS :=
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
ARCH_CXXFLAGS :=
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

# Always defined, so code can test it with a bare #if UBSAN. Instrumentation
# inflates stack frames noticeably, which the tighter embedded configs care about.
GLOBAL_DEFINES += UBSAN=$(if $(filter 1,$(UBSAN)),1,0)

# Anything added to GLOBAL_SRCDEPS will become a dependency of every source file in the system.
# Useful for header files that may be included by one or more source files.
GLOBAL_SRCDEPS := $(CONFIGHEADER)

# these need to be filled out by the project/target/platform rules.mk files
TARGET :=
PLATFORM :=
ARCH :=
ALLMODULES :=
RUST_CRATES :=

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
# setting V=1 on the command line or environment will also disable the silencing
ifeq (true,$(call TOBOOL,$(V)))
NOECHO :=
endif
NOECHO ?= @

# Any modules you want to explictly prevent from being used
DENY_MODULES :=

# Build modules with unit tests enabled
WITH_TESTS ?= false

# Set to 1 by platform/target rules.mk for targets with RAM well under ~1MB, so
# memory-hungry code (tests, etc) can size itself down or compile itself out.
LK_EMBEDDED ?= 0

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
# SUBARCH is only meaningful for some arches (riscv's 32/64, arm's arm/arm-m) and may be
# empty; the arch rules.mk has not been included yet so arch level defaults do not apply.
ifeq ($(MAKECMDGOALS), list-arch)
$(info ARCH = $(ARCH))
$(info SUBARCH = $(SUBARCH))
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

# Target tools are LK_CC, LK_LD and so on. The bare names are never read: they usually
# mean the host toolchain and are often exported in the environment for host builds.
$(call CHECK_RENAMED,CC:LK_CC LD:LK_LD OBJDUMP:LK_OBJDUMP OBJCOPY:LK_OBJCOPY \
    CPPFILT:LK_CXXFILT SIZE:LK_SIZE NM:LK_NM STRIP:LK_STRIP)

# default to no ccache
CCACHE ?=

TOOLCHAIN ?= gcc

ifeq ($(TOOLCHAIN),clang)
    # The clang on PATH and the tools installed beside it, or failing that the newest
    # /usr/lib/llvm-N; mixing releases breaks LTO (see the LTO_MODE check below).
    ifndef CLANG_BINDIR
        CLANG_BINDIR := $(patsubst %/,%,$(dir $(realpath $(shell which clang 2>/dev/null))))
        ifeq ($(CLANG_BINDIR),)
            CLANG_BINDIR := $(lastword $(shell ls -d /usr/lib/llvm-*/bin 2>/dev/null | sort -V))
        endif
    endif
    CLANG_PATH := $(if $(CLANG_BINDIR),$(wildcard $(CLANG_BINDIR)/clang),$(shell which clang 2>/dev/null))
    CLANG_CC := $(if $(CLANG_PATH),$(CLANG_PATH),clang)
    CLANG_TARGET ?= $(CLANG_ARCH_TRIPLE)
    LK_CC ?= $(CCACHE) $(CLANG_CC) --target=$(CLANG_TARGET)

    # DWARF 4 for compatibility with GNU binutils
    GLOBAL_COMPILEFLAGS += -gdwarf-4

    LLD_PATH := $(firstword $(if $(CLANG_BINDIR),$(wildcard $(CLANG_BINDIR)/ld.lld)) $(shell which ld.lld 2>/dev/null))
    ifeq ($(LK_LD),ld.lld)
        ifeq ($(LLD_PATH),)
            $(error ld.lld linker requested but not found. Please install lld package or set CLANG_BINDIR)
        endif
        override LK_LD := $(LLD_PATH)
    else
        LK_LD ?= $(if $(LLD_PATH),$(LLD_PATH),$(TOOLCHAIN_PREFIX)ld)
    endif

    OBJDUMP_PATH := $(firstword $(if $(CLANG_BINDIR),$(wildcard $(CLANG_BINDIR)/llvm-objdump)) $(shell which llvm-objdump 2>/dev/null))
    LK_OBJDUMP ?= $(if $(OBJDUMP_PATH),$(OBJDUMP_PATH),$(TOOLCHAIN_PREFIX)objdump)

    OBJCOPY_PATH := $(firstword $(if $(CLANG_BINDIR),$(wildcard $(CLANG_BINDIR)/llvm-objcopy)) $(shell which llvm-objcopy 2>/dev/null))
    LK_OBJCOPY ?= $(if $(OBJCOPY_PATH),$(OBJCOPY_PATH),$(TOOLCHAIN_PREFIX)objcopy)

    CXXFILT_PATH := $(firstword $(if $(CLANG_BINDIR),$(wildcard $(CLANG_BINDIR)/llvm-cxxfilt)) $(shell which llvm-cxxfilt 2>/dev/null))
    LK_CXXFILT ?= $(if $(CXXFILT_PATH),$(CXXFILT_PATH),$(TOOLCHAIN_PREFIX)c++filt)

    SIZE_PATH := $(firstword $(if $(CLANG_BINDIR),$(wildcard $(CLANG_BINDIR)/llvm-size)) $(shell which llvm-size 2>/dev/null))
    LK_SIZE ?= $(if $(SIZE_PATH),$(SIZE_PATH),$(TOOLCHAIN_PREFIX)size)

    NM_PATH := $(firstword $(if $(CLANG_BINDIR),$(wildcard $(CLANG_BINDIR)/llvm-nm)) $(shell which llvm-nm 2>/dev/null))
    LK_NM ?= $(if $(NM_PATH),$(NM_PATH),$(TOOLCHAIN_PREFIX)nm)

    STRIP_PATH := $(firstword $(if $(CLANG_BINDIR),$(wildcard $(CLANG_BINDIR)/llvm-strip)) $(shell which llvm-strip 2>/dev/null))
    LK_STRIP ?= $(if $(STRIP_PATH),$(STRIP_PATH),$(TOOLCHAIN_PREFIX)strip)
else
    LK_CC ?= $(CCACHE) $(TOOLCHAIN_PREFIX)gcc
    LK_LD ?= $(TOOLCHAIN_PREFIX)ld
    LK_OBJDUMP ?= $(TOOLCHAIN_PREFIX)objdump
    LK_OBJCOPY ?= $(TOOLCHAIN_PREFIX)objcopy
    LK_CXXFILT ?= $(TOOLCHAIN_PREFIX)c++filt
    LK_SIZE ?= $(TOOLCHAIN_PREFIX)size
    LK_NM ?= $(TOOLCHAIN_PREFIX)nm
    LK_STRIP ?= $(TOOLCHAIN_PREFIX)strip
endif

# Detect whether we are using ld.lld, assuming it is named ld.lld. If we don't
# detect ld.lld, we assume it's ld.bfd. This check can be refined in the future
# if we need to handle more cases (e.g. ld.gold).
#
# Match on the name rather than the version banner. An 'ld' that is really lld
# reports "LLD" and would be taken for a usable cross linker even when it is
# the Mach-O only flavor, as on macOS, enabling flags and code paths that then
# fail confusingly. LLD is only wanted here when it has been asked for by name
# (LK_LD=ld.lld), so that is what we look for.
LINKER_TYPE := $(if $(filter %ld.lld,$(notdir $(firstword $(LK_LD)))),lld,bfd)
$(info LINKER_TYPE=$(LINKER_TYPE))
# Detect whether we are compiling with GCC or Clang
COMPILER_TYPE := $(shell $(LK_CC) -v 2>&1 | grep -q "clang version" && echo clang || echo gcc)
$(info COMPILER_TYPE=$(COMPILER_TYPE))

# Find libgcc.a (or equivalent) for this compiler/multilib combination.
# This must happen here, now that LK_CC/TOOLCHAIN are fully resolved above --
# under TOOLCHAIN=clang this lets Clang report its own runtime library for
# targets where it has one; otherwise fall back to querying a matching
# cross GCC. (arch/*/rules.mk, included earlier, sets ARCH_COMPILEFLAGS and
# friends used below to select the right multilib variant.)
LIBGCC ?= $(shell $(LK_CC) $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) $(ARCH_COMPILEFLAGS_FLOAT) $(THUMBCFLAGS) $(GLOBAL_CFLAGS) -print-libgcc-file-name 2>/dev/null)
ifeq ($(wildcard $(LIBGCC)),)
LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) $(ARCH_COMPILEFLAGS_FLOAT) $(THUMBCFLAGS) $(GLOBAL_CFLAGS) -print-libgcc-file-name 2>/dev/null)
endif
# Freeze the result: without this, LIBGCC (set via ?=, a recursively-expanded
# variable) would re-run the shell command above on every later reference,
# against whatever GLOBAL_COMPILEFLAGS/ARCH_COMPILEFLAGS happen to look like
# at that point (more warning/ABI flags are appended below), which is both
# wasteful and fragile.
LIBGCC := $(strip $(LIBGCC))

# Optional link-time optimization: off, thin, or full.
# thin is Clang/lld-only (ThinLTO); GCC has no equivalent mode yet.
LTO_MODE ?= off
GLOBAL_LTO_COMPILEFLAGS :=
ifneq ($(LTO_MODE),off)
ifeq ($(COMPILER_TYPE),clang)
ifneq ($(LINKER_TYPE),lld)
$(error LTO_MODE=$(LTO_MODE) with TOOLCHAIN=clang requires ld.lld, which has native LTO support; got LK_LD=$(LK_LD))
endif
# LTO code generation runs inside ld.lld, which must come from the same LLVM release as the
# compiler: a newer one can read an older clang's target features differently (LLVM 21 drops
# M from clang 18's "+m,...,-zmmul") and quietly emit libcalls for every multiply and divide.
CLANG_MAJOR := $(firstword $(subst ., ,$(shell $(LK_CC) -dumpversion)))
LLD_MAJOR := $(shell $(LK_LD) --version | sed -n 's/.*LLD \([0-9][0-9]*\)\..*/\1/p')
ifneq ($(CLANG_MAJOR),$(LLD_MAJOR))
$(error LTO_MODE=$(LTO_MODE) needs clang and ld.lld from the same LLVM release; got clang $(CLANG_MAJOR) and ld.lld $(LLD_MAJOR) ($(LK_LD)), set CLANG_BINDIR)
endif
GLOBAL_LTO_COMPILEFLAGS += -flto=$(LTO_MODE)
else
$(error LTO_MODE=$(LTO_MODE) is not yet supported for COMPILER_TYPE=$(COMPILER_TYPE))
endif
endif

# Now that LK_CC is defined we can check if warning flags are supported and add
# them to GLOBAL_COMPILEFLAGS if they are.
ifeq ($(call is_warning_flag_supported,-Wnonnull-compare),yes)
GLOBAL_COMPILEFLAGS += -Wno-nonnull-compare
endif
# Note: -Wno-asm-operand-widths used to be added for arm64 here, on the premise
# that clang was wrong to want a 64-bit operand for msr. It is not wrong: MSR/MRS
# transfer all 64 bits and only accept an X register, so a 32-bit operand emits
# an x register whose upper half was never zeroed. ARM64_WRITE_SYSREG casts to
# uint64_t instead, which fixes the cause rather than hiding it.

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
$(info LK_DEBUG = $(LK_DEBUG))

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
	LK_EMBEDDED=$(LK_EMBEDDED) \
	$(addsuffix =1,$(addprefix WITH_,$(ALLMODULES)))

GLOBAL_DEFINES += LK_DEBUGLEVEL=$(LK_DEBUG)

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
GLOBAL_DEFINES += GLOBAL_CXXFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_CXXFLAGS))\"
GLOBAL_DEFINES += GLOBAL_ASMFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_ASMFLAGS))\"
GLOBAL_DEFINES += GLOBAL_LDFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_LDFLAGS))\"
GLOBAL_DEFINES += ARCH_COMPILEFLAGS=\"$(subst $(SPACE),_,$(ARCH_COMPILEFLAGS))\"
GLOBAL_DEFINES += ARCH_COMPILEFLAGS_FLOAT=\"$(subst $(SPACE),_,$(ARCH_COMPILEFLAGS_FLOAT))\"
GLOBAL_DEFINES += ARCH_COMPILEFLAGS_NOFLOAT=\"$(subst $(SPACE),_,$(ARCH_COMPILEFLAGS_NOFLOAT))\"
GLOBAL_DEFINES += ARCH_CFLAGS=\"$(subst $(SPACE),_,$(ARCH_CFLAGS))\"
GLOBAL_DEFINES += ARCH_CXXFLAGS=\"$(subst $(SPACE),_,$(ARCH_CXXFLAGS))\"
GLOBAL_DEFINES += ARCH_ASMFLAGS=\"$(subst $(SPACE),_,$(ARCH_ASMFLAGS))\"
GLOBAL_DEFINES += ARCH_LDFLAGS=\"$(subst $(SPACE),_,$(ARCH_LDFLAGS))\"
GLOBAL_DEFINES += LTO_MODE=\"$(LTO_MODE)\"
GLOBAL_DEFINES += GLOBAL_LTO_COMPILEFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_LTO_COMPILEFLAGS))\"
GLOBAL_DEFINES += TOOLCHAIN_PREFIX=\"$(subst $(SPACE),_,$(TOOLCHAIN_PREFIX))\"
GLOBAL_DEFINES += TOOLCHAIN=\"$(subst $(SPACE),_,$(TOOLCHAIN))\"
GLOBAL_DEFINES += LK_LD=\"$(subst $(SPACE),_,$(LK_LD))\"
GLOBAL_DEFINES += LIBGCC=\"$(subst $(SPACE),_,$(LIBGCC))\"

# Bare names from the old module format, which the build never reads. Values from the
# environment are ignored: CFLAGS, CPPFLAGS and CXXFLAGS are often exported for host
# builds.
ifneq ($(call SET_IN_MAKE,OBJS),)
$(warning OBJS=$(OBJS))
$(error OBJS is not empty, please convert to new module format)
endif
ifneq ($(call SET_IN_MAKE,OPTFLAGS),)
$(warning OPTFLAGS=$(OPTFLAGS))
$(error OPTFLAGS is not empty, please use GLOBAL_OPTFLAGS or MODULE_OPTFLAGS)
endif
ifneq ($(call SET_IN_MAKE,CFLAGS),)
$(warning CFLAGS=$(CFLAGS))
$(error CFLAGS is not empty, please use GLOBAL_CFLAGS or MODULE_CFLAGS)
endif
ifneq ($(call SET_IN_MAKE,CPPFLAGS),)
$(warning CPPFLAGS=$(CPPFLAGS))
$(error CPPFLAGS is not empty, please use GLOBAL_CXXFLAGS or MODULE_CXXFLAGS)
endif
ifneq ($(call SET_IN_MAKE,CXXFLAGS),)
$(warning CXXFLAGS=$(CXXFLAGS))
$(error CXXFLAGS is not empty, please use GLOBAL_CXXFLAGS or MODULE_CXXFLAGS)
endif

# C++ flags are *_CXXFLAGS; *_CPPFLAGS conventionally means preprocessor flags.
$(call CHECK_RENAMED,GLOBAL_CPPFLAGS:GLOBAL_CXXFLAGS ARCH_CPPFLAGS:ARCH_CXXFLAGS \
    EXTRA_MODULE_CPPFLAGS:EXTRA_MODULE_CXXFLAGS)

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
	rm -f -- $(ALLOBJS) $(DEPS) $(GENERATED) $(OUTBIN) $(OUTELF) $(OUTELF).lst $(OUTELF).debug.lst $(OUTELF).sym $(OUTELF).sym.sorted $(OUTELF).size $(OUTELF).hex $(OUTELF).dump $(COMPDB)
	rm -f -- $(addsuffix .json,$(ALLOBJS))

install: all
	scp $(OUTBIN) 192.168.0.4:/tftpboot

tags: $(BUILDDIR)/srcfiles.txt $(BUILDDIR)/include_paths.txt
	$(info generating tags)
	@ctags -L $<

# Join the per-object entries the compile recipes wrote into one compilation database
# for this build. Remade on every build, since the set of objects can change without
# any of them being rebuilt; the file is only replaced when its contents change.
# Paths are relative to BUILDDIR to keep the recipe's command line short.
COMPDB := $(BUILDDIR)/compile_commands.json
COMPDB_ENTRIES := $(patsubst $(BUILDDIR)/%,%,$(addsuffix .json,$(sort $(ALLOBJS))))
all:: $(COMPDB)

$(COMPDB): $(ALLOBJS) compdb-force
	$(info generating $@)
	@$(MKDIR)
	$(NOECHO)cd -- "$(BUILDDIR)" || exit 1; missing=0; \
	for f in $(COMPDB_ENTRIES); do \
		if [ -f $$f ]; then echo $$f; else missing=$$((missing+1)); fi; \
	done > compile_commands.list; \
	{ echo '['; xargs cat < compile_commands.list | sed '$$!s/$$/,/'; echo ']'; } > compile_commands.json.tmp; \
	rm -f -- compile_commands.list; \
	$(call TESTANDREPLACEFILE,compile_commands.json.tmp,compile_commands.json) >/dev/null; \
	if [ $$missing -ne 0 ]; then \
		echo "warning: $$missing objects in $(BUILDDIR) have no compile command recorded;" \
			"they were built before compile_commands.json support. 'make $(PROJECT) clean' and rebuild to include them."; \
	fi

# Point ./compile_commands.json at this build's database, for tools that look for it in
# the source root. Repoint it by running 'make <project> compdb' for another project.
compdb: $(COMPDB)
	$(info linking compile_commands.json -> $(COMPDB))
	$(NOECHO)ln -sf $(COMPDB) compile_commands.json

compdb-force:

.PHONY: all clean compdb compdb-force install list-arch list-toolchain tags

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
