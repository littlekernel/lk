LOCAL_MAKEFILE:=$(MAKEFILE_LIST)

ifeq ($(MAKECMDGOALS),spotless)
spotless:
	rm -rf build-*
else

ifndef LKROOT
$(error please define LKROOT to the root of the lk build system)
endif

-include local.mk
include make/macros.mk

# If one of our goals (from the commandline) happens to have a
# matching project/goal.mk, then we should re-invoke make with
# that project name specified...

project-name := $(firstword $(MAKECMDGOALS))

ifneq ($(project-name),)
ifneq ($(strip $(foreach d,$(LKINC),$(wildcard $(d)/project/$(project-name).mk))),)
do-nothing := 1
$(MAKECMDGOALS) _all: make-make
make-make:
	@PROJECT=$(project-name) $(MAKE) -rR -f $(LOCAL_MAKEFILE) $(filter-out $(project-name), $(MAKECMDGOALS))

.PHONY: make-make
endif
endif

ifeq ($(do-nothing),)

ifeq ($(PROJECT),)
$(error No project specified.  Use "make projectname" or put "PROJECT := projectname" in local.mk)
endif

DEBUG ?= 2

BUILDROOT ?= .
BUILDDIR := $(BUILDROOT)/build-$(PROJECT)
OUTBIN := $(BUILDDIR)/lk.bin
OUTELF := $(BUILDDIR)/lk.elf
CONFIGHEADER := $(BUILDDIR)/config.h

GLOBAL_INCLUDES := $(BUILDDIR) $(LKROOT)/include $(addsuffix /include,$(LKINC))
GLOBAL_OPTFLAGS ?= -Os
GLOBAL_COMPILEFLAGS := -g -finline -W -Wall -Wno-multichar -Wno-unused-parameter -Wno-unused-function -include $(CONFIGHEADER)
GLOBAL_CFLAGS := --std=gnu99 -Werror-implicit-function-declaration -Wstrict-prototypes
#GLOBAL_CFLAGS += -Werror
GLOBAL_CPPFLAGS := -fno-exceptions -fno-rtti -fno-threadsafe-statics
#GLOBAL_CPPFLAGS += -Weffc++
GLOBAL_ASMFLAGS := -DASSEMBLY
GLOBAL_LDFLAGS :=

GLOBAL_COMPILEFLAGS += -ffunction-sections -fdata-sections
GLOBAL_LDFLAGS += --gc-sections

# top level rule
all:: $(OUTBIN) $(OUTELF).lst $(OUTELF).debug.lst $(OUTELF).sym $(OUTELF).size $(OUTELF).hex

# master module object list
ALLOBJS_MODULE :=

# master object list (for dep generation)
ALLOBJS :=

# a linker script needs to be declared in one of the project/target/platform files
LINKER_SCRIPT :=

# anything you add here will be deleted in make clean
GENERATED := $(CONFIGHEADER)

# anything added to GLOBAL_DEFINES will be put into $(BUILDDIR)/config.h
GLOBAL_DEFINES := LK=1

# Anything added to SRCDEPS will become a dependency of every source file in the system.
# Useful for header files that may be included by one or more source files.
SRCDEPS := $(CONFIGHEADER)

# these need to be filled out by the project/target/platform rules.mk files
TARGET :=
PLATFORM :=
ARCH :=
ALLMODULES :=

# add any external module dependencies
MODULES := $(EXTERNAL_MODULES)

# any rules you put here will also be built by the system before considered being complete
EXTRA_BUILDDEPS :=

# any rules you put here will be depended on in clean builds
EXTRA_CLEANDEPS :=

# any objects you put here get linked with the final image
EXTRA_OBJS :=

# if someone defines this, the build id will be pulled into lib/version
BUILDID ?=

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

$(info PROJECT = $(PROJECT))
$(info PLATFORM = $(PLATFORM))
$(info TARGET = $(TARGET))

include arch/$(ARCH)/rules.mk
include top/rules.mk

# recursively include any modules in the MODULE variable, leaving a trail of included
# modules in the ALLMODULES list
include make/recurse.mk

# any extra top level build dependencies that someone declared
all:: $(EXTRA_BUILDDEPS)

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

DEPS := $(ALLOBJS:%o=%d)

# prefix all of the paths in GLOBAL_INCLUDES with -I
GLOBAL_INCLUDES := $(addprefix -I,$(GLOBAL_INCLUDES))

# test for some old variables
ifneq ($(INCLUDES),)
$(error INCLUDES variable set, please move to GLOBAL_INCLUDES: $(INCLUDES))
endif
ifneq ($(DEFINES),)
$(error DEFINES variable set, please move to GLOBAL_DEFINES: $(DEFINES))
endif

# default to no ccache
CCACHE ?=
CC := $(CCACHE) $(TOOLCHAIN_PREFIX)gcc
LD := $(TOOLCHAIN_PREFIX)ld
OBJDUMP := $(TOOLCHAIN_PREFIX)objdump
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
CPPFILT := $(TOOLCHAIN_PREFIX)c++filt
SIZE := $(TOOLCHAIN_PREFIX)size
NM := $(TOOLCHAIN_PREFIX)nm

# put all of the global build flags in config.h to force a rebuild if any change
GLOBAL_DEFINES += GLOBAL_INCLUDES=\"$(subst $(SPACE),_,$(GLOBAL_INCLUDES))\"
GLOBAL_DEFINES += GLOBAL_COMPILEFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_COMPILEFLAGS))\"
GLOBAL_DEFINES += GLOBAL_OPTFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_OPTFLAGS))\"
GLOBAL_DEFINES += GLOBAL_CFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_CFLAGS))\"
GLOBAL_DEFINES += GLOBAL_CPPFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_CPPFLAGS))\"
GLOBAL_DEFINES += GLOBAL_ASMFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_ASMFLAGS))\"
GLOBAL_DEFINES += GLOBAL_LDFLAGS=\"$(subst $(SPACE),_,$(GLOBAL_LDFLAGS))\"

# comment out or override if you want to see the full output of each command
NOECHO ?= @

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

# the logic to compile and link stuff is in here
include make/build.mk

clean: $(EXTRA_CLEANDEPS)
	rm -f $(ALLOBJS) $(DEPS) $(GENERATED) $(OUTBIN) $(OUTELF) $(OUTELF).lst $(OUTELF).debug.lst $(OUTELF).sym $(OUTELF).size $(OUTELF).hex

install: all
	scp $(OUTBIN) 192.168.0.4:/tftproot

# generate a config.h file with all of the GLOBAL_DEFINES laid out in #define format
configheader:

$(CONFIGHEADER): configheader
	$(call MAKECONFIGHEADER,$@,GLOBAL_DEFINES)

# Empty rule for the .d files. The above rules will build .d files as a side
# effect. Only works on gcc 3.x and above, however.
%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif

.PHONY: configheader
endif

endif # make spotless
