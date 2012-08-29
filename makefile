ifeq ($(MAKECMDGOALS),spotless)
spotless:
	rm -rf build-*
else

-include local.mk
include make/macros.mk

# If one of our goals (from the commandline) happens to have a
# matching project/goal.mk, then we should re-invoke make with
# that project name specified...

project-name := $(firstword $(MAKECMDGOALS))

ifneq ($(project-name),)
ifneq ($(wildcard project/$(project-name).mk),)
do-nothing := 1
$(MAKECMDGOALS) _all: make-make
make-make:
	@PROJECT=$(project-name) $(MAKE) $(filter-out $(project-name), $(MAKECMDGOALS))
endif
endif

ifeq ($(do-nothing),)

ifeq ($(PROJECT),)
$(error No project specified.  Use "make projectname" or put "PROJECT := projectname" in local.mk)
endif

DEBUG ?= 2

BUILDDIR := build-$(PROJECT)
OUTBIN := $(BUILDDIR)/lk.bin
OUTELF := $(BUILDDIR)/lk
CONFIGHEADER := $(BUILDDIR)/config.h

INCLUDES := -I$(BUILDDIR) -Iinclude
GLOBAL_OPTFLAGS ?= -Os
GLOBAL_COMPILEFLAGS := -g -fno-builtin -finline -W -Wall -Wno-multichar -Wno-unused-parameter -Wno-unused-function -include $(CONFIGHEADER)
GLOBAL_CFLAGS := --std=c99 -Werror-implicit-function-declaration
#GLOBAL_CFLAGS += -Werror
GLOBAL_CPPFLAGS := -fno-exceptions -fno-rtti -fno-threadsafe-statics
#GLOBAL_CPPFLAGS += -Weffc++
GLOBAL_ASMFLAGS := -DASSEMBLY
GLOBAL_LDFLAGS := 

GLOBAL_COMPILEFLAGS += -ffunction-sections -fdata-sections
GLOBAL_LDFLAGS += -gc-sections

# top level rule
all:: $(OUTBIN) $(OUTELF).lst $(OUTELF).debug.lst $(OUTELF).sym $(OUTELF).size

# master module object list
ALLOBJS_MODULE :=

# master object list (for dep generation)
ALLOBJS :=

# a linker script needs to be declared in one of the project/target/platform files
LINKER_SCRIPT := 			

# anything you add here will be deleted in make clean
GENERATED := $(CONFIGHEADER)

# anything added to DEFINES will be put into $(BUILDDIR)/config.h
DEFINES := LK=1				

# Anything added to SRCDEPS will become a dependency of every source file in the system.
# Useful for header files that may be included by one or more source files.
SRCDEPS := $(CONFIGHEADER)

# these need to be filled out by the project/target/platform rules.mk files
TARGET :=
PLATFORM :=
ARCH :=
ALLMODULES :=
MODULES :=

# any rules you put here will also be built by the system before considered being complete
EXTRA_BUILDDEPS := 

# any rules you put here will be depended on in clean builds
EXTRA_CLEANDEPS := 

include project/$(PROJECT).mk
include target/$(TARGET)/rules.mk
include platform/$(PLATFORM)/rules.mk
include arch/$(ARCH)/rules.mk
include platform/rules.mk
include target/rules.mk
include kernel/rules.mk
include dev/rules.mk
include app/rules.mk

# recursively include any modules in the MODULE variable, leaving a trail of included
# modules in the ALLMODULES list
include make/recurse.mk

# any extra top level build dependencies that someone declared
all:: $(EXTRA_BUILDDEPS)

# add some automatic configuration defines
DEFINES += \
	PROJECT_$(PROJECT)=1 \
	TARGET_$(TARGET)=1 \
	PLATFORM_$(PLATFORM)=1 \
	ARCH_$(ARCH)=1 \
	$(addsuffix =1,$(addprefix WITH_,$(ALLMODULES)))

# debug build?
ifneq ($(DEBUG),)
DEFINES += \
	DEBUG=$(DEBUG)
endif

DEPS := $(ALLOBJS:%o=%d)

#$(warning DEPS=$(DEPS))

# default to no ccache
CCACHE ?= 
CC := $(CCACHE) $(TOOLCHAIN_PREFIX)gcc
LD := $(TOOLCHAIN_PREFIX)ld
OBJDUMP := $(TOOLCHAIN_PREFIX)objdump
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
CPPFILT := $(TOOLCHAIN_PREFIX)c++filt
SIZE := $(TOOLCHAIN_PREFIX)size
NM := $(TOOLCHAIN_PREFIX)nm

# comment out or override if you want to see the full output of each command
NOECHO ?= @

#$(warning ALLMODULE_OBJS=$(ALLMODULE_OBJS))

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
	rm -f $(ALLOBJS) $(DEPS) $(GENERATED) $(OUTBIN) $(OUTELF) $(OUTELF).lst

install: all
	scp $(OUTBIN) 192.168.0.4:/tftproot

# generate a config.h file with all of the DEFINES laid out in #define format
configheader:

$(CONFIGHEADER): configheader
	$(call MAKECONFIGHEADER,$@,DEFINES)

# Empty rule for the .d files. The above rules will build .d files as a side
# effect. Only works on gcc 3.x and above, however.
%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif

.PHONY: configheader
endif

endif # make spotless
