# LK Kernel Development Guide

LK is a small, SMP-aware embedded OS kernel designed for supervisor mode on diverse 32/64-bit architectures. It's used extensively in embedded systems, including Android bootloaders. Written primarily in C and assembly, with limited C++ (no STL, no exceptions).

## Architecture Overview

### Hierarchical Build System
LK uses a 4-layer modular build system:

1. **Project** (`project/*.mk`) - Top-level configuration defining which modules to include
   - Example: `project/qemu-virt-arm64-test.mk` includes shell, filesystem, networking modules
   - Projects include other project fragments: `include project/virtual/test.mk`

2. **Target** (`target/*.mk`) - Board-specific configuration combining platform + hardware details
   - Defines memory layout: `MEMBASE`, `MEMSIZE`, `KERNEL_BASE`
   - GPIO configs, peripheral addresses for specific boards

3. **Platform** (`platform/*/`) - SOC/system-level support (qemu-virt, stm32f4xx, etc.)
   - Hardware initialization, device tree handling, platform-specific drivers

4. **Architecture** (`arch/*/`) - CPU-specific low-level code (arm64, riscv, x86, etc.)
   - MMU setup, exception handling, context switching, atomic ops

### Module System Pattern
Every component is a module with a `rules.mk` file:

```make
LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/foo.c \
    $(LOCAL_DIR)/bar.c

MODULE_DEPS += \
    lib/libc \
    kernel

MODULE_OPTIONS := extra_warnings  # Enables stricter compiler flags

include make/module.mk
```

**Key points:**
- `MODULE := $(LOCAL_DIR)` is required - sets module name to directory path
- `MODULE_DEPS` creates dependency tree, automatically included in build
- `MODULE_OPTIONS`: `extra_warnings` adds strict checks, `float` enables FP compilation
- Module include paths auto-added: `$(MODULE)/include/` becomes available globally
- Always use `$(LOCAL_DIR)` prefix for source paths
- Must `include make/module.mk` at end of `rules.mk` to finalize the module definition
- All MODULE_* variables are cleared after inclusion, preventing leakage between modules

## Critical Build Patterns

### Building Projects
```bash
# Build specific project (creates build-<project>/ directory)
make qemu-virt-arm64-test

# Or just use project name as target
make PROJECT=qemu-virt-arm64-test

# Override heap implementation
make qemu-virt-arm64-test LK_HEAP_IMPLEMENTATION=cmpctmalloc

# Debug builds (default DEBUG=2, set to 0 for release)
make qemu-virt-arm64-test DEBUG=0

# Clean specific project
make build-qemu-virt-arm64-test clean

# Clean everything
make spotless

# Build all projects (for CI/verification)
scripts/buildall -q -e -r  # quiet, errors-as-warnings, release builds
```

### Running Tests
Scripts in `scripts/` launch QEMU with appropriate flags:
```bash
# ARM64 (4KB pages)
scripts/do-qemuarm -6

# ARM64 with 64KB pages
scripts/do-qemuarm -6 -P 64k

# ARM64 with KVM/HVF acceleration (only if on ARM64 host)
scripts/do-qemuarm -6 -k

# RISC-V 32-bit in machine mode
scripts/do-qemuriscv

# RISC-V 64-bit in supervisor mode and paging
scripts/do-qemuriscv -6S

# x86-32
scripts/do-qemux86

# x86-64
scripts/do-qemux86 -6

# x86-64 with KVM acceleration (only if on x86-64 host)
scripts/do-qemux86 -6 -k

# With various devices (disk, network, display)
scripts/do-qemux86 -6 -n -d disk.img -g
```

The do-qemu* scripts auto-build before launching QEMU.

### Running all unit tests

```bash
# Run all unit tests for ARM64 architecture
./scripts/run-qemu-boot-tests.py --arch arm64

# For all architectures
./scripts/run-qemu-boot-tests.py
```

## Code Conventions

### Style (enforced by `.clang-format`)
- **4 space indentation**, no tabs, no trailing whitespace
- **Pointer alignment right**: `void *ptr` not `void* ptr`
- **K&R braces**: `if (x) {` not `if (x)\n{`
- **Header guards**: Always use `#pragma once` (never `#ifndef` guards)
- Short if/loops allowed: `if (foo) return;` is acceptable
- 100 Column soft limit (line breaks preferred before 100 chars)

### Compiler Warnings
- Base flags: `-Wall -Werror=return-type -Wshadow -Wdouble-promotion`
- C-specific: `-Werror-implicit-function-declaration -Wstrict-prototypes`
- C++: `-fno-exceptions -fno-rtti -fno-threadsafe-statics --std=c++14`
- All code compiled with `-ffreestanding` (no hosted environment assumptions)
- `MODULE_OPTIONS := extra_warnings` adds `-Wmissing-declarations -Wredundant-decls`

### Common Patterns

#### Registering Console Commands
Commands appear in shell when `app/shell` module is included:

```c
#include <lk/console_cmd.h>

static int my_command(int argc, const console_cmd_args *argv) {
    printf("hello from %s\n", argv[0].str);
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("mytest", "my test command", &my_command)
STATIC_COMMAND_END(mytest);
```

Console commands are placed in linker section `"console_cmds"` and auto-registered at runtime.

**Note:** If `lib/console` not in build, these macros expand to nothing and the code should not be emitted.

#### Defining Applications
Apps start automatically at boot (unless `APP_FLAG_NO_AUTOSTART`):

```c
#include <app.h>

static void my_app_init(const struct app_descriptor *app) {
    // Called during boot, before threads start
}

static void my_app_entry(const struct app_descriptor *app, void *args) {
    // Runs in separate thread
    printf("app %s running\n", app->name);
}

APP_START(myapp)
    .init = my_app_init,
    .entry = my_app_entry,
APP_END
```

Apps are placed in linker section `"apps"` and auto-discovered at runtime.

#### Memory Management Variants
Select heap implementation in project or via make:

```make
# In project.mk or command line
LK_HEAP_IMPLEMENTATION ?= dlmalloc  # default
# LK_HEAP_IMPLEMENTATION ?= cmpctmalloc  # compact allocator
# LK_HEAP_IMPLEMENTATION ?= miniheap     # simple bump allocator

# Controlled in lib/heap/rules.mk
```

#### Virtual Memory Usage
Architectures with MMU set `WITH_KERNEL_VM ?= 1` in `arch/*/rules.mk`:
- Enables `kernel/vm` instead of `kernel/novm`
- Requires `KERNEL_ASPACE_BASE/SIZE` and `USER_ASPACE_BASE/SIZE` definitions
- For ARM64 architecture:
  - Page size configurable: `ARM64_PAGE_SIZE` (4096, 16384, 65536) on ARM64 architecture
  - Different projects for different page sizes: `qemu-virt-arm64-64k-test`
- All other architecture use 4KB pages by default.

### Global Defines
Architecture/platform rules set defines via `GLOBAL_DEFINES +=`:
- Goes into `$(BUILDDIR)/config.h` (auto-generated, auto-included)
- Example: `GLOBAL_DEFINES += WITH_SMP=1 SMP_MAX_CPUS=8`
- Common defines: `MEMBASE`, `MEMSIZE`, `KERNEL_BASE`, `IS_64BIT`, `WITH_KERNEL_VM`

## Common Workflows

### Adding a New Module
1. Create directory under appropriate location (`lib/`, `dev/`, `app/`)
2. Create `rules.mk` with module definition
3. Add source files, set `MODULE_DEPS` for dependencies to other modules from this module
4. Include new module in project/target/platform as needed
5. Headers in `<module>/include/` are globally accessible

### Adding Platform Support
1. Create `platform/<name>/` directory
2. Define `platform/<name>/rules.mk` with `PLATFORM := <name>`
3. Implement: `platform_early_init()`, `platform_init()`, `platform_halt()`
4. Create target in `target/<board>/rules.mk` that includes platform
5. Create project in `project/<board>-test.mk`

### Debugging
- Multiple DEBUG build levels, controlled via `DEBUG` make variable:
  - `DEBUG=0`: no DEBUG_ASSERT, dprintf only for ALWAYS level
  - `DEBUG=1`: DEBUG_ASSERT enabled, dprintf at INFO and ALWAYS
  - `DEBUG=2`: DEBUG_ASSERT enabled, dprintf at DEBUG, INFO, ALWAYS
  - `DEBUG=3`: DEBUG_ASSERT enabled, dprintf at DEBUG, INFO, ALWAYS, some extra runtime checks.
- 'DEBUG=2' is default
- QEMU scripts support GDB: `scripts/do-qemuarm -6 -s -S` (wait for GDB on :1234)
- Print output via `printf()` goes to console (UART or QEMU serial)
- dprintf levels:
  - `dprintf(ALWAYS, "message")` - always printed
  - `dprintf(INFO, "message")` - printed in DEBUG>=1
  - `dprintf(DEBUG, "message")` - printed in DEBUG>=2
- `kernel/debug.c` provides: `hexdump()`, `panic()`, `ASSERT()`

### Testing
- Some Shell commands test individual subsystems interactively
- `app/tests/` contains some unit test commands to run through the shell
- `lib/unittest` contains a unit test framework that other libraries can use to define tests.
  - Tests are auto-discovered and run with `ut all` on the command line shell, or automatically
    at boot time if `RUN_UNITTESTS_AT_BOOT` is defined at build time.

## Key Files Reference

- `engine.mk` - Core build engine, processes modules
- `make/module.mk` - Module system implementation
- `arch/*/rules.mk` - Architecture definitions (critical for porting)
- `kernel/thread.c` - Threading and scheduler implementation
- `kernel/vm/` - Virtual memory subsystem (for MMU architectures)
- `lib/libc/` - Minimal C library (string, stdio, stdlib basics)
- `top/` - Top level module in the system. Contains the kernel's lk_main() system init routines.
   Also contains top level lk/ include headers.

For detailed architecture info, see `docs/` (threading, VMM, platform-specific guides).