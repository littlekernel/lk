-include local.mk
include macros.mk

PROJECT ?= armemu-test
DEBUG ?= false

BUILDDIR := build-$(PROJECT)
OUTBIN := $(BUILDDIR)/lk.bin
OUTELF := $(BUILDDIR)/lk
CONFIGHEADER := $(BUILDDIR)/config.h

INCLUDES := -Iinclude
CFLAGS := -O2 -g -fno-builtin -finline -W -Wall -Wno-multichar -Wno-unused-parameter -Wno-unused-function -include $(CONFIGHEADER)
#CFLAGS += -Werror
CPPFLAGS := -fno-exceptions -fno-rtti -fno-threadsafe-statics
#CPPFLAGS += -Weffc++
ASMFLAGS := -DASSEMBLY
LDFLAGS := 

CFLAGS += -ffunction-sections -fdata-sections
LDFLAGS += -gc-sections

# top level rule
all:: $(OUTBIN) $(OUTELF).lst $(OUTELF).debug.lst $(OUTELF).sym

# the following three object lists are identical except for the ordering
# which is bootobjs, kobjs, objs
BOOTOBJS :=	
KOBJS :=
OBJS :=

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
LIBS := libc
APPS :=
DEVS :=

# any rules you put here will also be built by the system before considered being complete
EXTRA_BUILDDEPS :=

# any rules you put here will be depended on in clean builds
EXTRA_CLEANDEPS :=

include project/$(PROJECT)/rules.mk
include target/$(TARGET)/rules.mk
include platform/$(PLATFORM)/rules.mk
include arch/$(ARCH)/rules.mk
include platform/rules.mk
include target/rules.mk
include kernel/rules.mk
include dev/rules.mk

DEVS := $(sort $(DEVS))
LIBS := $(sort $(LIBS))
APPS := $(sort $(APPS))

include $(addsuffix /rules.mk,$(addprefix dev/,$(DEVS)))
include $(addsuffix /rules.mk,$(addprefix lib/,$(LIBS)))
include $(addsuffix /rules.mk,$(addprefix app/,$(APPS)))

# any extra top level build dependencies that someone declared
all:: $(EXTRA_BUILDDEPS)

ALLOBJS := \
	$(BOOTOBJS) \
	$(KOBJS) \
	$(OBJS)

# add some automatic configuration defines
DEFINES += \
	PROJECT_$(PROJECT)=1 \
	TARGET_$(TARGET)=1 \
	PLATFORM_$(PLATFORM)=1 \
	ARCH_$(ARCH)=1 \
	$(addsuffix =1,$(addprefix WITH_DEV_,$(DEVS))) \
	$(addsuffix =1,$(addprefix WITH_LIB_,$(LIBS))) \
	$(addsuffix =1,$(addprefix WITH_APP_,$(APPS)))

# debug build?
ifeq ($(DEBUG),true)
DEFINES += \
	DEBUG=1
endif

ALLOBJS := $(addprefix $(BUILDDIR)/,$(ALLOBJS))

DEPS := $(ALLOBJS:%o=%d)

CC := $(TOOLCHAIN_PREFIX)gcc
LD := $(TOOLCHAIN_PREFIX)ld
OBJDUMP := $(TOOLCHAIN_PREFIX)objdump
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
CPPFILT := $(TOOLCHAIN_PREFIX)c++filt
SIZE := $(TOOLCHAIN_PREFIX)size
NM := $(TOOLCHAIN_PREFIX)nm

include build.mk

clean: $(EXTRA_CLEANDEPS)
	rm -f $(ALLOBJS) $(DEPS) $(GENERATED) $(OUTBIN) $(OUTELF) $(OUTELF).lst

spotless:
	rm -rf build-*

install: all
	scp $(OUTBIN) 192.168.0.4:/tftproot

# generate a config.h file with all of the DEFINES laid out in #define format
configheader:

$(CONFIGHEADER): configheader
	@$(MKDIR)
	@echo generating $@
	@rm -f $(CONFIGHEADER).tmp; \
	echo \#ifndef __CONFIG_H > $(CONFIGHEADER).tmp; \
	echo \#define __CONFIG_H >> $(CONFIGHEADER).tmp; \
	for d in `echo $(DEFINES) | tr [:lower:] [:upper:]`; do \
		echo "#define $$d" | sed "s/=/\ /g;s/-/_/g;s/\//_/g" >> $(CONFIGHEADER).tmp; \
	done; \
	echo \#endif >> $(CONFIGHEADER).tmp; \
	if [ -f "$(CONFIGHEADER)" ]; then \
		if cmp "$(CONFIGHEADER).tmp" "$(CONFIGHEADER)"; then \
			rm -f $(CONFIGHEADER).tmp; \
		else \
			mv $(CONFIGHEADER).tmp $(CONFIGHEADER); \
		fi \
	else \
		mv $(CONFIGHEADER).tmp $(CONFIGHEADER); \
	fi

# Empty rule for the .d files. The above rules will build .d files as a side
# effect. Only works on gcc 3.x and above, however.
%.d:

ifeq ($(filter $(MAKECMDGOALS), clean), )
-include $(DEPS)
endif

.PHONY: configheader

