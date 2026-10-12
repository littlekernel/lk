LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x10000000
MEMBASE := 0x20000000
MEMSIZE := 0x00042000
# can be overridden by target

ARCH := arm
ARM_CPU := cortex-m0plus

LK_EMBEDDED := 1

GLOBAL_DEFINES += \
	MEMSIZE=$(MEMSIZE) \
	ARM_CM_SET_VTOR=1 \
	PICO_COMPILER_IS_GNU=1 \
	PICO_NO_BINARY_INFO=1 \
	PICO_NO_FPGA_CHECK=1 \
	PICO_NO_HARDWARE=0 \
	PICO_ON_DEVICE=1 \
	PICO_RP2040=1 \

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/vectab.c

MODULE_SRCS += \
	$(LKROOT)/external/platform/pico/common/hardware_claim/claim.c \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_clocks/clocks.c \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_gpio/gpio.c \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_pll/pll.c \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_timer/timer.c \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_ticks/ticks.c \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_uart/uart.c \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_watchdog/watchdog.c \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_xosc/xosc.c \
	$(LKROOT)/external/platform/pico/rp2_common/pico_runtime_init/runtime_init_clocks.c

GLOBAL_INCLUDES += \
	$(LKROOT)/external/platform/pico/common/pico_base_headers/include \
	$(LKROOT)/external/platform/pico/common/pico_binary_info/include \
	$(LKROOT)/external/platform/pico/common/hardware_claim/include \
	$(LKROOT)/external/platform/pico/rp2040/pico_platform/include \
	$(LKROOT)/external/platform/pico/rp2040/hardware_regs/include \
	$(LKROOT)/external/platform/pico/rp2040/hardware_structs/include \
	$(LKROOT)/external/platform/pico/rp2_common/pico_platform_compiler/include \
	$(LKROOT)/external/platform/pico/rp2_common/pico_platform_sections/include \
	$(LKROOT)/external/platform/pico/rp2_common/pico_platform_panic/include \
	$(LKROOT)/external/platform/pico/rp2_common/pico_platform_common/include \
	$(LKROOT)/external/platform/pico/rp2_common/pico_bootrom/include \
	$(LKROOT)/external/platform/pico/rp2_common/boot_bootrom_headers/include \
	$(LKROOT)/external/platform/pico/rp2_common/pico_flash/include \
	$(LKROOT)/external/platform/pico/rp2_common/pico_runtime/include \
	$(LKROOT)/external/platform/pico/rp2_common/pico_runtime_init/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_base/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_clocks/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_gpio/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_irq/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_pll/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_resets/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_sync/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_sync_spin_lock/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_timer/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_ticks/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_uart/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_watchdog/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_vreg/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_xosc/include \
	$(LKROOT)/external/platform/pico/rp2_common/hardware_boot_lock/include

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

# take the result of the build and generate a uf2 file
UF2BIN := $(basename $(OUTBIN)).uf2
UF2CONV_TOOL := $(LOCAL_DIR)/tools/uf2conv.py
FAMILY_ID := 0xe48bff56 # UF2 family id
$(UF2BIN): $(OUTBIN) $(UF2CONV_TOOL)
	@$(MKDIR)
	$(NOECHO)echo generating $@; \
	$(UF2CONV_TOOL) -b $(ROMBASE) -f $(FAMILY_ID) -c -o $@ $<

EXTRA_BUILDDEPS += $(UF2BIN)
GENERATED += $(UF2BIN)

include make/module.mk
