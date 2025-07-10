# LK Source Tree Structure

This document describes the organization and structure of the Little Kernel (LK) source code repository.

## Top-Level Directories

### `/arch/`
Architecture-specific code for different CPU architectures:
- `arm/` - ARM 32-bit and cortex-m support
- `arm64/` - ARM 32-bit and 64-bit support
- `riscv/` - RISC-V 32-bit and 64-bit support
- `m68k/` - Motorola 68000 support
- `microblaze/` - Microblaze support
- `mips/` - MIPS support
- `or1k/` - OpenRISC 1000 support
- `x86/` - x86 and x86_64 support
- `vax/` - VAX support (experimental)

Each architecture directory contains:
- CPU-specific initialization code
- Memory management unit (MMU) setup
- Exception and interrupt handling
- Low level context switching and thread support
- Architecture-specific assembly code

### `/kernel/`
Core kernel functionality:
- `mutex.c`, `semaphore.c` - Synchronization primitives
- `mp.c` - Symmetric multiprocessing support
- `timer.c` - Kernel timers
- `event.c` - Event handling
- `port.c` - Message passing
- `thread.c` - Threading system and scheduler
- `vm/` - Virtual memory management
- `novm/` - Memory management for architectures without an mmu

### `/lib/`
Library and utility code:
- `libc/` - Basic C library functions
- `libm/` - Math library
- `bio/` - Block I/O layer
- `fs/` - File system support
- `heap/` - Dynamic memory allocation
- `io/` - I/O utilities
- `console/` - Console and debugging support
- `cksum/` - Checksum algorithms
- `debug/` - Debugging utilities
- many more helper libraries and utility routines

### `/dev/`
Device drivers organized by category:
- `bus/` - Bus drivers (PCI, etc.)
- `display/` - Display and graphics drivers
- `interrupt/` - Interrupt controller drivers
- `net/` - Network device drivers
- `timer/` - Hardware timer drivers
- `uart/` - Serial port drivers
- `usb/` - USB device side drivers
- `virtio/` - VirtIO device support

### `/platform/`
Platform specific code (not all listed):
- `qemu-*/` - QEMU virtual machine platforms
- `fvp-base/` - ARM Fixed Virtual Platform support
- `stm32*/` - Support for various STM32 SOCs
- Hardware-specific board support packages
- Platform initialization and device tree handling

### `/target/`
Target specific code:
- A specific board or combination of platform and architecture. For example:
- `stm32f4-discovery`
- `pc-x86`
- `pico`

### `/app/`
Application and test code:
- Test applications for kernel functionality
- Sample applications demonstrating LK features
- Shell and command-line interface
- Network applications and services
- Location where general embedded device logic go

### `/external/`
Third-party code and libraries:
- External dependencies
- Imported code from other projects
- Third-party drivers and utilities

### `/scripts/`
Build and development scripts:
- `do-qemu*` - QEMU launch scripts for different architectures
- `fetch-toolchains.py` - Toolchain download utility
- Build automation and testing scripts

### `/tools/`
Development and build tools:
- Custom build utilities
- Code generation tools
- Development helpers

### `/docs/`
Documentation:
- Architecture documentation
- Platform-specific guides
- API documentation
- Development guides

## Key Files

### `/make/`
Build system components:
- Makefile fragments for different components
- Architecture and platform build rules
- Toolchain configuration

### Root Directory Files
- `Makefile` - Main build system entry point
- `engine.mk` - Core build engine
- `project/*.mk` - Project configuration files
- `target/*.mk` - Target-specific build configurations

## Build System Organization

The LK build system is modular and component-based:

1. **Projects** (`project/*.mk`) define high-level configurations
2. **Targets** (`target/*.mk`) specify platform and architecture combinations
3. **Modules** (scattered throughout source tree) define buildable components
4. **Rules** (`make/*.mk`) provide build logic for different file types

Each directory can contain:
- `rules.mk` - Module definition and build rules
- Source files (`.c`, `.cpp`, `.S`)
- Local headers and includes

## Module System

LK uses a module-based build system where each component is defined as a module:

```make
# Example module definition in rules.mk
LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/file1.c \
    $(LOCAL_DIR)/file2.c

MODULE_DEPS += \
    lib/libc \
    kernel

include make/module.mk
```

This allows for:
- Selective compilation of components
- Dependency management between modules
- Platform-specific module inclusion/exclusion
- Modular kernel configuration

## Platform and Architecture Interaction

The relationship between platforms and architectures:

1. **Architecture** (`/arch/`) provides CPU-specific low-level support
2. **Platform** (`/platform/`) provides system-specific support (generally system-on-chip)
3. **Target** (`/target/`) combines a specific platform with board specific customization (GPIOs, etc)
4. **Project** (`/project/`) selects the target and a list of modules to include for a complete system

Example target `stm32f4-discovery` combines:
- Platform: `stm32f4xx` (support for STM32 F4 series SOC)
- Architecture: `arm` (32-bit ARM)
- Modules: Selected via project configuration

This structure allows LK to support diverse hardware while maintaining code reuse and modularity.
