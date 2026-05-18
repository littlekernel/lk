HAS_XDFTOOL := $(shell which xdftool)

ifeq ($(HAS_XDFTOOL),)
$(error ERROR: 'xdftool' not found in PATH. Please install amitools)
endif

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := m68k
# NOTE: All chip models are supported, but 060 needs explicit targeting
M68K_CPU := 68000

LK_HEAP_IMPLEMENTATION ?= dlmalloc

# Optional: Include useful libs
MODULE_DEPS += \
		lib/cbuf \
		lib/console \
		lib/gfxconsole \
		app/tests \

# Your platform-specific source files
MODULE_SRCS += $(LOCAL_DIR)/display.c
MODULE_SRCS += $(LOCAL_DIR)/platform.c
MODULE_SRCS += $(LOCAL_DIR)/serial.c
MODULE_SRCS += $(LOCAL_DIR)/keyboard.c
MODULE_SRCS += $(LOCAL_DIR)/timer.c
MODULE_SRCS += $(LOCAL_DIR)/power.c
MODULE_SRCS += $(LOCAL_DIR)/stage1.S
MODULE_SRCS += $(LOCAL_DIR)/stage2.S

# RAM layout
MEMBASE ?= 0x400
MEMSIZE ?= 0x7c800 # Target 512KB chip ram for now

# Optional useful defines
GLOBAL_DEFINES += PLATFORM_SUPPORTS_PANIC_SHELL=1
GLOBAL_DEFINES += PLATFORM_HAS_DYNAMIC_TIMER=1  # unless you add a real timer
GLOBAL_DEFINES += NOVM_DEFAULT_ARENA=0
GLOBAL_DEFINES += ARCH_DO_RELOCATION=1
GLOBAL_DEFINES += CONSOLE_HAS_INPUT_BUFFER=1

STAGE1_ELF := $(BUILDDIR)/platform/amiga/stage1.S.o
STAGE1_RAW := $(BUILDDIR)/platform/amiga/stage1.raw

STAGE2_ELF := $(BUILDDIR)/platform/amiga/stage2.S.o
STAGE2_RAW := $(BUILDDIR)/platform/amiga/stage2.raw

BOOTLOADER := $(BUILDDIR)/platform/amiga/bootloader.raw

KERNEL_IMAGE := $(OUTBIN)
ADF_IMAGE := $(basename $(KERNEL_IMAGE)).adf

$(STAGE1_RAW): $(STAGE1_ELF)
	m68k-elf-objcopy -O binary $< $@
	truncate -s 512 $@; \

# Build and pad stage2 payload
$(STAGE2_RAW): $(STAGE2_ELF)
	m68k-elf-objcopy -O binary $< $@
	truncate -s 512 $@

$(BOOTLOADER): $(STAGE1_RAW) $(STAGE2_RAW)
	dd if=/dev/zero bs=1024 count=1 of=$(BOOTLOADER); \
	
	dd if=$(STAGE1_RAW) of=$(BOOTLOADER) bs=1 seek=0 conv=notrunc; \
	dd if=$(STAGE2_RAW) of=$(BOOTLOADER) bs=1 seek=512 conv=notrunc; \
	truncate -s 1012 $@; \

$(ADF_IMAGE): $(KERNEL_IMAGE) $(BOOTLOADER)
	rm -f $@
	xdftool $@ create
	xdftool $@ format lk ffs
	xdftool $@ boot write $(BOOTLOADER)
	@KSIZE=$$(stat -c %s "$(KERNEL_IMAGE)"); \
	echo -n "$$KSIZE"; \
	printf '%08x' "$$KSIZE" | xxd -r -p | dd of=$@ bs=1024 seek=1 conv=notrunc; \
	dd if=$(KERNEL_IMAGE) of=$@ bs=512 seek=4 conv=notrunc; \

EXTRA_BUILDDEPS += $(ADF_IMAGE)
GENERATED += $(ADF_IMAGE) $(BOOTLOADER) $(STAGE1_RAW) $(STAGE2_RAW) $(STAGE1_ELF) $(STAGE2_ELF)

include make/module.mk
