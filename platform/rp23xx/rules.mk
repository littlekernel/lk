LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_COMPILEFLAGS += -Wno-error

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x10000000
MEMBASE := 0x20000000
MEMSIZE := 0x00082000
# can be overridden by target

ARCH := arm
ARM_CPU := cortex-m33

GLOBAL_DEFINES += \
	MEMSIZE=$(MEMSIZE) \
	ARM_CM_SET_VTOR=1 \
	PICO_COMPILER_IS_GNU=1 \
	PICO_NO_BINARY_INFO=1 \
	PICO_NO_FPGA_CHECK=1 \
	PICO_NO_HARDWARE=0 \
	PICO_ON_DEVICE=1 \
	PICO_RP2350=1 \

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/vectab.c \
	$(LOCAL_DIR)/usb/usbc.c

MODULE_SRCS += \
	external/platform/pico/common/hardware_claim/claim.c \
	external/platform/pico/rp2_common/hardware_clocks/clocks.c \
	external/platform/pico/rp2_common/hardware_gpio/gpio.c \
	external/platform/pico/rp2_common/hardware_pll/pll.c \
	external/platform/pico/rp2_common/hardware_timer/timer.c \
	external/platform/pico/rp2_common/hardware_ticks/ticks.c \
	external/platform/pico/rp2_common/hardware_uart/uart.c \
	external/platform/pico/rp2_common/hardware_watchdog/watchdog.c \
	external/platform/pico/rp2_common/hardware_xosc/xosc.c \
	external/platform/pico/rp2_common/pico_runtime_init/runtime_init_clocks.c

GLOBAL_INCLUDES += \
	external/platform/pico/common/pico_base_headers/include \
	external/platform/pico/common/pico_binary_info/include \
	external/platform/pico/common/hardware_claim/include \
	external/platform/pico/rp2350/pico_platform/include \
	external/platform/pico/rp2350/hardware_regs/include \
	external/platform/pico/rp2350/hardware_structs/include \
	external/platform/pico/rp2_common/pico_platform_compiler/include \
	external/platform/pico/rp2_common/pico_platform_sections/include \
	external/platform/pico/rp2_common/pico_platform_panic/include \
	external/platform/pico/rp2_common/pico_platform_common/include \
	external/platform/pico/rp2_common/pico_bootrom/include \
	external/platform/pico/rp2_common/boot_bootrom_headers/include \
	external/platform/pico/common/boot_picobin_headers/include \
	external/platform/pico/rp2_common/pico_flash/include \
	external/platform/pico/rp2_common/pico_runtime/include \
	external/platform/pico/rp2_common/pico_runtime_init/include \
	external/platform/pico/rp2_common/hardware_base/include \
	external/platform/pico/rp2_common/hardware_clocks/include \
	external/platform/pico/rp2_common/hardware_gpio/include \
	external/platform/pico/rp2_common/hardware_irq/include \
	external/platform/pico/rp2_common/hardware_pll/include \
	external/platform/pico/rp2_common/hardware_resets/include \
	external/platform/pico/rp2_common/hardware_sync/include \
	external/platform/pico/rp2_common/hardware_sync_spin_lock/include \
	external/platform/pico/rp2_common/hardware_timer/include \
	external/platform/pico/rp2_common/hardware_ticks/include \
	external/platform/pico/rp2_common/hardware_uart/include \
	external/platform/pico/rp2_common/hardware_watchdog/include \
	external/platform/pico/rp2_common/hardware_vreg/include \
	external/platform/pico/rp2_common/hardware_xosc/include \
	external/platform/pico/rp2_common/hardware_boot_lock/include \
	external/platform/pico/rp2_common/cmsis/stub/CMSIS/Core/Include \
	external/platform/pico/rp2_common/cmsis/stub/CMSIS/Device/RP2350/Include

# use a two segment memory layout, where all of the read-only sections
# of the binary reside in rom, and the read/write are in memory. The
# ROMBASE, MEMBASE, and MEMSIZE make variables are required to be set
# for the linker script to be generated properly.
#
LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	dev/usb \
	arch/arm/arm-m/systick \
	lib/cbuf

# take the result of the build and generate a uf2 file
UF2BIN := $(basename $(OUTBIN)).uf2
UF2CONV_TOOL := $(LOCAL_DIR)/../rp20xx/tools/uf2conv.py
FAMILY_ID := 0xe48bff59 # UF2 family id (RP2350 ARM Secure)
$(UF2BIN): $(OUTBIN) $(UF2CONV_TOOL)
	@$(MKDIR)
	$(NOECHO)echo generating $@; \
	$(UF2CONV_TOOL) -b $(ROMBASE) -f $(FAMILY_ID) -c --abs-block -o $@ $<

# Alternatively, if picotool is available on the host:
#   picotool uf2 convert $(OUTELF) $(UF2BIN) --platform rp2350 --family $(FAMILY_ID) --abs-block

EXTRA_BUILDDEPS += $(UF2BIN)
GENERATED += $(UF2BIN)

include make/module.mk
