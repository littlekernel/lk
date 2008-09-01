#
# The TARGET is expected to indicate which *specific* AT91SAM7 chip
# is being used, since features and memory vary from chip to chip
#
#             chip       ram   rom    EMAC  CAN  
# AT91CHIP := sam7s64    16k   64k    N     N    
# AT91CHIP := sam7s256   64k   256k   N     N
# AT91CHIP := sam7x256   64k   256k   Y     Y
#

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x0
MEMBASE := 0x200000

TMP_CFG := bad
ifeq ($(AT91CHIP), sam7x256)
DEFINES += AT91_SAM7X=1 
DEFINES += AT91_RAMSIZE=65536
DEFINES += AT91_ROMSIZE=262144
MEMSIZE := 65536
TMP_CFG := ok
endif
ifeq ($(AT91CHIP), sam7s256)
DEFINES += AT91_SAM7S=1 
DEFINES += AT91_RAMSIZE=65536
DEFINES += AT91_ROMSIZE=262144
MEMSIZE := 65536
TMP_CFG := ok
endif
ifeq ($(AT91CHIP), sam7s64)
DEFINES += AT91_SAM7S=1 
DEFINES += AT91_RAMSIZE=16384
DEFINES += AT91_ROMSIZE=65536
MEMSIZE := 16384
TMP_CFG := ok
endif

ifeq ($(TMP_CFG), bad)
$(error The AT91SAM7 platform requires AT91CHIP be set by the target)
endif

LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := arm
ARM_CPU := arm7tdmi

DEFINES += AT91_MCK_MHZ=48000000

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/platform_early.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/init_clock.o \
	$(LOCAL_DIR)/init_clock_48mhz.o \
	$(LOCAL_DIR)/mux.o \
	$(LOCAL_DIR)/emac_dev.o

# use a two segment memory layout, where all of the read-only sections 
# of the binary reside in rom, and the read/write are in memory. The 
# ROMBASE, MEMBASE, and MEMSIZE make variables are required to be set 
# for the linker script to be generated properly.
#
LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

