ifndef ARCH_arm_TOOLCHAIN_INCLUDED
ARCH_arm_TOOLCHAIN_INCLUDED := 1

# try to find the toolchain
ifndef ARCH_arm_TOOLCHAIN_PREFIX
ARCH_arm_TOOLCHAIN_PREFIX := arm-eabi-
FOUNDTOOL=$(shell which $(ARCH_arm_TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
ARCH_arm_TOOLCHAIN_PREFIX := arm-elf-
FOUNDTOOL=$(shell which $(ARCH_arm_TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
ARCH_arm_TOOLCHAIN_PREFIX := arm-none-eabi-
FOUNDTOOL=$(shell which $(ARCH_arm_TOOLCHAIN_PREFIX)gcc)
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

endif
endif
endif
ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set ARCH_arm_TOOLCHAIN_PREFIX or add it to your path)
endif
endif


ifeq ($(ARM_CPU),cortex-m3)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
endif
ifeq ($(ARM_CPU),cortex-m4)
ARCH_arm_COMPILEFLAGS += -mcpu=$(ARM_CPU)
endif
ifeq ($(ARM_CPU),cortex-m4f)
ARCH_arm_COMPILEFLAGS += -mcpu=cortex-m4 -mfloat-abi=softfp
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

endif
