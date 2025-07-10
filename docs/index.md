# LK (Little Kernel) Documentation

Welcome to the LK (Little Kernel) documentation. LK is a small operating system designed for embedded systems, offering a lightweight kernel with threading, memory management, and device drivers.

## Table of Contents

### Getting Started
- [Getting Started Guide](getting_started.md) - Quick start guide for building and running LK
- [TODO List](todo.md) - Current development tasks and known issues

### Core Kernel Documentation
- [Threading and Scheduler System](threading_and_scheduler.md) - Comprehensive guide to LK's preemptive multithreading system
- [Blocking Primitives](blocking_primitives.md) - Synchronization primitives (mutexes, semaphores, events, ports, spinlocks)
- [VMM Overview](vmm_overview.md) - Virtual Memory Management system architecture and implementation
- [Source Tree Structure](source_tree_structure.md) - Organization and structure of the LK source code repository

### Platform-Specific Documentation

#### ARM Fixed Virtual Platform (FVP)
- [FVP Base - How to Run](fvp-base/how_to_run.md) - Build and run instructions for FVP Base platform
- [FVP Base - Internal Overview](fvp-base/internal.md) - Boot sequence, memory layout, and peripheral details

### Development and Testing
- [Network Setup for QEMU](lk_tap.md) - Setting up tun/tap networking for QEMU testing

## Architecture Overview

LK is designed as a modular kernel with the following key components:

- **Threading System**: Preemptive multithreading with priority-based scheduling
- **Memory Management**: Physical memory management (PMM) and virtual memory management (VMM)
- **Synchronization**: Comprehensive set of blocking primitives for thread coordination
- **Device Support**: Drivers for common embedded peripherals
- **Platform Abstraction**: Support for multiple CPU architectures and platforms

## Supported Architectures

- ARM (32-bit and 64-bit)
- RISC-V (32-bit and 64-bit)
- x86/x86_64
- Motorola 68000
- Microblaze
- MIPS
- OpenRISC 1000
- VAX (experimental)

## Supported Platforms

- QEMU virtual platforms (ARM, RISC-V, x86)
- ARM Fixed Virtual Platform (FVP)
- Various embedded development boards
- Custom hardware platforms

## Key Features

- **Real-time capable**: Priority-based preemptive scheduler with real-time thread support
- **Memory protection**: Virtual memory management with address space isolation
- **SMP support**: Symmetric multiprocessing for multi-core systems
- **Modular design**: Component-based architecture for easy customization
- **Small footprint**: Designed for resource-constrained embedded systems

## Contributing

LK is an open-source project. For the latest development status and to contribute:

- [GitHub Repository](https://github.com/littlekernel/lk)
- [Issue Tracker](https://github.com/littlekernel/lk/issues)

## Quick Start

1. Install or build a recent version of qemu. Make sure qemu-system-riscv64 is in your path.

2. Clone the repository:

   ```bash
   git clone https://github.com/littlekernel/lk
   cd lk
   ```

3. Download a toolchain:

   ```bash
   scripts/fetch-toolchains.py --prefix riscv64-elf
   export PATH=$PWD/toolchain/riscv64-elf-15.1.0-Linux-x86_64/bin:$PATH
   ```

4. Build and run:

   ```bash
   make qemu-virt-riscv64-test
   scripts/do-qemuriscv -6S
   ```

For detailed instructions, see the [Getting Started Guide](getting_started.md).

This will get you a interactive prompt into LK which is running in qemu
arm64 machine 'virt' emulation. type 'help' for commands.

Note: for ubuntu x86-64
sudo apt-get install gcc-aarch64-linux-gnu
or fetch a prebuilt toolchain from
https://newos.org/toolchains/aarch64-elf-15.1.0-Linux-x86_64.tar.xz

### Building with LLVM-based toolchains

To build LK with a LLVM-based toolchain you will have to manually specify the compiler and linker in the environemnt.
Unlike GCC clang is a cross-compiler by default, so the target needs to be specified as part of the CC/CXX/CPP variables.
For example, assuming LLVM is in `/opt/llvm/bin/`, the following command will work to build for 64-bit RISC-V:

```bash
gmake qemu-virt-riscv64-test 'CC=/opt/llvm/bin/clang --target=riscv64-unknown-elf' 'CPP=/opt/llvm/bin/clang-cpp --target=riscv64-unknown-elf' 'CXX=/opt/llvm/bin/clang++ --target=riscv64-unknown-elf' 'LD=/opt/llvm/bin/ld.lld' TOOLCHAIN_PREFIX=/opt/llvm/bin/llvm- CPPFILT=/opt/llvm/bin/llvm-cxxfilt
```

TOOLCHAIN_PREFIX can be set to use the LLVM binutils, but due to the different naming of `llvm-cxxfilt` vs `c++filt` it needs to be set explicitly.

To build for AArch64 the command looks similar, just with a different `--target=` flag.

```bash
gmake qemu-virt-arm64-test 'CC=/opt/llvm/bin/clang --target=aarch64-unknown-elf' 'CPP=/opt/llvm/bin/clang-cpp --target=aarch64-unknown-elf' 'CXX=/opt/llvm/bin/clang++ --target=aarch64-unknown-elf' 'LD=/opt/llvm/bin/ld.lld' TOOLCHAIN_PREFIX=/opt/llvm/bin/llvm- CPPFILT=/opt/llvm/bin/llvm-cxxfilt
```

---

## [TODO](todo.md)

*This documentation covers the core aspects of the LK kernel. For specific implementation details, refer to the source code and individual documentation files.*
