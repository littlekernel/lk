ifndef ARCH_arm_TOOLCHAIN_INCLUDED
ARCH_arm_TOOLCHAIN_INCLUDED := 1

# try to find the toolchain
ifndef ARCH_arm_TOOLCHAIN_PREFIX

$(info $(TOOLCHAIN_PREFIX))
# if TOOLCHAIN_PREFIX is not empty, try to use it first
ifneq ($(TOOLCHAIN_PREFIX),)
ARCH_arm_TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREFIX)
FOUNDTOOL=$(shell which $(ARCH_arm_TOOLCHAIN_PREFIX)gcc)
endif

# try a series of common arm toolchain prefixes in the path
ifeq ($(FOUNDTOOL),)
ARCH_arm_TOOLCHAIN_PREFIX := arm-eabi-
FOUNDTOOL=$(shell which $(ARCH_arm_TOOLCHAIN_PREFIX)gcc)
endif
ifeq ($(FOUNDTOOL),)
ARCH_arm_TOOLCHAIN_PREFIX := arm-elf-
FOUNDTOOL=$(shell which $(ARCH_arm_TOOLCHAIN_PREFIX)gcc)
endif
ifeq ($(FOUNDTOOL),)
ARCH_arm_TOOLCHAIN_PREFIX := arm-none-eabi-
FOUNDTOOL=$(shell which $(ARCH_arm_TOOLCHAIN_PREFIX)gcc)
endif
ifeq ($(FOUNDTOOL),)
ARCH_arm_TOOLCHAIN_PREFIX := arm-linux-gnueabi-
FOUNDTOOL=$(shell which $(ARCH_arm_TOOLCHAIN_PREFIX)gcc)

# Set no stack protection if we found our gnueabi toolchain. We don't
# need it.
#
# Stack protection is default in this toolchain and we get such errors
# final linking stage:
#
# undefined reference to `__stack_chk_guard'
# undefined reference to `__stack_chk_fail'
# undefined reference to `__stack_chk_guard'
#
ifneq (,$(findstring arm-linux-gnueabi-,$(FOUNDTOOL)))
        ARCH_arm_COMPILEFLAGS += -fno-stack-protector
endif
endif # arm-linux-gnueabi-

else
FOUNDTOOL=$(shell which $(ARCH_arm_TOOLCHAIN_PREFIX)gcc)
endif # ARCH_arm_TOOLCHAIN_PREFIX

ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set ARCH_arm_TOOLCHAIN_PREFIX or add it to your path)
endif

ifeq ($(ARM_CPU),cortex-m0)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
ARCH_arm_COMPILEFLAGS += -mthumb -mfloat-abi=soft
endif
ifeq ($(ARM_CPU),cortex-m0plus)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
ARCH_arm_COMPILEFLAGS += -mthumb -mfloat-abi=soft
endif
ifeq ($(ARM_CPU),cortex-m3)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
endif
ifeq ($(ARM_CPU),cortex-m4)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
endif
ifeq ($(ARM_CPU),cortex-m7)
# use cortex-m4 for now until better general toolchain support
ARCH_arm_COMPILEFLAGS += -mcpu=cortex-m4
endif
ifeq ($(ARM_CPU),cortex-m7-fpu-sp-d16)
# use cortex-m4 for now until better general toolchain support
ARCH_arm_COMPILEFLAGS += -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp
endif
ifeq ($(ARM_CPU),cortex-m4f)
ARCH_arm_COMPILEFLAGS += -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp
endif
ifeq ($(ARM_CPU),cortex-a7)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
ARCH_arm_COMPILEFLAGS += -mfpu=vfpv3 -mfloat-abi=softfp
endif
ifeq ($(ARM_CPU),cortex-a8)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
ARCH_arm_COMPILEFLAGS += -mfpu=neon -mfloat-abi=softfp
endif
ifeq ($(ARM_CPU),cortex-a9)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
endif
ifeq ($(ARM_CPU),cortex-a9-neon)
ARCH_arm_COMPILEFLAGS += -mcpu=cortex-a9
# XXX cannot enable neon right now because compiler generates
# neon code for 64bit integer ops
ARCH_arm_COMPILEFLAGS += -mfpu=vfpv3 -mfloat-abi=softfp
endif
ifeq ($(ARM_CPU),cortex-a15)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
ifneq ($(ARM_WITHOUT_VFP_NEON),true)
ARCH_arm_COMPILEFLAGS += -mfpu=vfpv3 -mfloat-abi=softfp
endif
endif
ifeq ($(ARM_CPU),arm1136j-s)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
endif
ifeq ($(ARM_CPU),arm1176jzf-s)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
endif
ifeq ($(ARM_CPU),armemu)
ARCH_arm_COMPILEFLAGS += -march=armv7-a
endif

endif
