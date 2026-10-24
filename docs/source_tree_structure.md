# LK Source Tree Structure

This document describes the organization of the LK source tree. [AGENTS.md](../AGENTS.md)
covers the build system in more depth.

## Top-Level Directories

### `/arch/`
Architecture-specific code:
- `arm/` - ARM 32-bit: Cortex-A (`arm/`) and Cortex-M (`arm-m/`)
- `arm64/` - ARM 64-bit
- `riscv/` - RISC-V 32-bit and 64-bit, machine or supervisor mode
- `m68k/` - Motorola 680x0
- `mips/` - MIPS32
- `x86/` - x86 32-bit and x86-64
- `test/` - architecture unit tests (MMU, FPU context switching)

An architecture directory holds CPU initialization, exception and interrupt entry, context
switching, cache and barrier operations, and on architectures with an MMU, the page table
code. The per-arch `rules.mk` sets the compiler flags and, where needed, `toolchain.mk`
picks the toolchain prefix.

### `/top/`
The top level module: `lk_main()` and the boot sequence (`main.c`), init hooks (`init.c`),
and the `lk/` headers shared by everything (`lk/err.h`, `lk/list.h`, `lk/console_cmd.h`,
`lk/init.h`, ...).

### `/kernel/`
Core kernel:
- `thread.c` - threads and the scheduler
- `mutex.c`, `semaphore.c`, `event.c` - blocking primitives
- `port.c` - message ports
- `timer.c` - kernel timers
- `mp.c` - multiprocessor support
- `vm/` - physical and virtual memory management, for architectures with an MMU
- `novm/` - page allocator for architectures without one
- `test/` - unit tests for the primitives above

### `/lib/`
Libraries, each its own module, pulled in by projects as needed. Some of the larger ones:
- `libc/`, `libcpp/` - minimal C library, freestanding subset of the C++ standard library
- `lktl/` - LK's own C++ library (namespace `lk`), header only
- `heap/` - heap front end with selectable allocators (miniheap, dlmalloc, cmpctmalloc)
- `console/` - command line console; `app/shell` puts a prompt on it
- `bio/`, `partition/` - block I/O layer and partition scanning (MBR, GPT)
- `fs/` - filesystem layer with FAT, ext2, 9p, spifs and memfs
- `minip/` - TCP/IP stack
- `fdtwalk/` - helpers that configure the system from a device tree
- `uefi/` - UEFI loader for running UEFI applications such as the Android GBL
- `unittest/` - the unit test framework behind `ut all`
- `backtrace/`, `symtab/` - frame pointer stack walks and in-kernel symbol lookup

### `/dev/`
Device drivers, by category:
- `block/` - AHCI, IDE
- `bus/` - PCI and PCIe
- `interrupt/` - interrupt controllers (ARM GIC, RISC-V PLIC, ...)
- `net/` - network interface drivers
- `timer/` - timer drivers
- `uart/` - serial ports
- `usb/` - USB device side stack
- `virtio/` - virtio block, net, gpu, rng and 9p
- `fbcon/`, `gpio/`, `gpio_i2c/`, `keys/`, `power/`, `cache/` - smaller drivers and interfaces

### `/platform/`
SoC or system level support: memory map, interrupt controller and timer setup, device
discovery. Examples:
- `qemu-virt-arm/`, `qemu-virt-riscv/`, `qemu-virt-m68k/`, `pc/` - emulated and PC platforms
- `fvp-base/` - ARM Fixed Virtual Platform
- `stm32*/`, `nrf5*/`, `rp2*/`, `lpc*/` - microcontroller families
- `jh7110/`, `spacemit-k1/`, `eic7700/`, `sifive/` - RISC-V SoCs

### `/target/`
A board: one platform plus board specific details such as pin assignments, clocks and
memory size. Each lives in `target/<board>/rules.mk`. For example `stm32f4-discovery`
selects the `stm32f4xx` platform, which in turn selects the `arm` architecture.

### `/project/`
The buildable configurations. `project/<name>.mk` picks a target and lists the modules to
include; `make <name>` builds it. `project/virtual/` holds fragments shared by many projects
(tests, filesystems, networking) and `project/target/` holds per-board fragments.

### `/app/`
Apps start automatically at boot through `APP_START`: the shell, network services, the
`lkboot` loader, test and benchmark commands in `app/tests/`, and the place for the main
logic of an embedded product.

### `/external/`
Code that originates outside LK, laid out to mirror the tree (`external/lib`,
`external/arch`, `external/platform`). The build searches `external/` like the root, so
`external/lib/cksum` is the `lib/cksum` module. Third-party code here keeps its own license.

### `/rust/`
Rust crates linked into the kernel when a project is built with `USE_RUST=1`.

### `/scripts/`
- `do-qemu*` - build a project and launch it under QEMU
- `run-qemu-boot-tests.py` - build, boot and run the unit tests across architectures
- `buildall` - build every project
- `fetch-toolchains.py` - download prebuilt toolchains
- `check-module-deps.py` - find `#include`s not covered by `MODULE_DEPS`

### `/tools/`
Host tools: `mkimage` and `lkboot` for LK boot images, `bin2h.py`, `nettool.py`.

### `/docs/`
Longer documentation, indexed in [index.md](index.md).

## Build System Files

- `makefile` - entry point; re-invokes make on `engine.mk` with the search paths
- `engine.mk` - core build engine: project, target, platform and arch resolution, toolchain
  selection, global flags
- `make/` - the shared machinery: `module.mk` (module definition), `compile.mk`,
  `build.mk` (final link), `macros.mk`, `recurse.mk`, `help.mk`
- `local.mk` - optional, untracked, silently included for local settings
- `lk_inc.mk.example` - template for building LK from an outer directory

## Build System Organization

A build is assembled in four layers:

1. **Project** (`project/<name>.mk`) - selects a target and lists modules
2. **Target** (`target/<board>/rules.mk`) - board details; selects a platform
3. **Platform** (`platform/<soc>/rules.mk`) - SoC support; selects an architecture
4. **Architecture** (`arch/<arch>/rules.mk`) - CPU support and compiler flags

Every component is a module: a directory with a `rules.mk` that lists its sources and its
dependencies on other modules.

```make
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

Only modules reachable from the project's list are built. A module's `include/` directory
is added to the global include path.
