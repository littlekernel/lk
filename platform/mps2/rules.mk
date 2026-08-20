LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm

# Machine specific configuration. The memory layouts come from the qemu board
# models in hw/arm/mps2.c and hw/arm/mps2-tz.c.
#
# Every one of these machines boots by fetching the initial stack pointer and
# reset vector from address 0, so the vector table (and with it the read only
# half of the image) has to live at ROMBASE 0. The read/write half goes in
# whichever RAM the machine has the most of.
ifeq ($(MPS2_MACHINE),an385)
# Cortex-M3. 4MB ZBT SSRAM1 at 0, 4MB SSRAM2&3 at 0x20000000.
ARM_CPU := cortex-m3
ROMBASE := 0x00000000
MEMBASE := 0x20000000
MEMSIZE ?= 0x400000
else ifeq ($(MPS2_MACHINE),an386)
# Cortex-M4, same memory map as the an385.
ARM_CPU := cortex-m4f
ROMBASE := 0x00000000
MEMBASE := 0x20000000
MEMSIZE ?= 0x400000
else ifeq ($(MPS2_MACHINE),an500)
# Cortex-M7, same memory map as the an385.
ARM_CPU := cortex-m7-fpu-sp-d16
ROMBASE := 0x00000000
MEMBASE := 0x20000000
MEMSIZE ?= 0x400000
else ifeq ($(MPS2_MACHINE),an505)
# Cortex-M33 in an IoTKit. 4MB ssram at 0, and 2MB + 2MB of contiguous ssram
# at 0x28000000. The 0x20000000 region is only the 32KB of SSE internal sram
# here, so the data segment goes in the larger block instead.
#
# Unlike the other machines this one comes out of reset with VTOR pointing at
# 0x10000000, the secure alias of the ssram at 0. qemu reads the initial stack
# pointer and reset vector out of the loaded image by its link address, so the
# image has to be linked at the alias the cpu will actually look at rather than
# at 0. Everything is addressed through the secure alias to match.
ARM_CPU := cortex-m33f
ROMBASE := 0x10000000
MEMBASE := 0x38000000
MEMSIZE ?= 0x400000
else ifeq ($(MPS2_MACHINE),an547)
# Cortex-M55 in an SSE-300. 512KB of ITCM at 0 holds the image, so this is the
# tightest of the bunch. The 4MB sram at 0x21000000 is used instead of the
# 512KB DTCM at 0x20000000 to leave the data segment some room.
ARM_CPU := cortex-m55
ROMBASE := 0x00000000
MEMBASE := 0x21000000
MEMSIZE ?= 0x400000
else
$(error unknown MPS2_MACHINE $(MPS2_MACHINE), set it in the project target fragment)
endif

GLOBAL_DEFINES += \
	MPS2_MACHINE_$(call UC,$(MPS2_MACHINE))=1

# Use semihosting to talk to the host qemu: it is the only way to get a command
# line onto these machines and the only way to make `poweroff` actually stop
# the emulator. Turn it off to run under a bare qemu with no
# -semihosting-config, where the trap instruction would fault instead.
MPS2_WITH_SEMIHOSTING ?= 1
GLOBAL_DEFINES += \
	MPS2_WITH_SEMIHOSTING=$(MPS2_WITH_SEMIHOSTING)

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/power.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/vectab.c

# use a two segment memory layout, where all of the read-only sections
# of the binary reside in rom, and the read/write are in memory. The
# ROMBASE, MEMBASE, and MEMSIZE make variables are required to be set
# for the linker script to be generated properly.
#
LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	arch/arm/arm-m/systick \
	lib/cbuf

ifeq ($(MPS2_WITH_SEMIHOSTING),1)
MODULE_DEPS += \
	lib/cmdline \
	lib/semihosting
endif

include make/module.mk
