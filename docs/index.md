# LK Documentation

The [README](../README.md) has the overview of LK: what it is, where it is used, the
supported architectures and platforms, and a quick start. This page indexes the longer
documents in the tree.

## Getting started

- [Getting Started Guide](getting_started.md) - fetching a toolchain, the first build, and
  setting up an out-of-tree project with its own app
- [AGENTS.md](../AGENTS.md) - a dense reference for the build system (projects, targets,
  platforms, modules), build options, test runners and code conventions

## Kernel

- [Threading and Scheduler](threading_and_scheduler.md) - threads, priorities, the run queues
  and SMP scheduling
- [Blocking Primitives](blocking_primitives.md) - mutexes, semaphores, events, ports and
  spinlocks
- [VMM Overview](vmm_overview.md) - boot mappings and the boot allocator, the PMM, address
  spaces, the arch MMU contract, and user address spaces
- [Intrusive Lists](list.md) - `lk/list.h` and its C++ wrapper in `lib/lktl`
- [Filesystem Layer](fs.md) - the mount namespace, the vnode interface, and how to write a
  filesystem
- [Source Tree Structure](source_tree_structure.md) - what lives where in the repository

## Booting, networking and debugging

- [Booting via UEFI (x86-64)](uefi_boot.md) - the in-kernel EFI stub, tested with QEMU and OVMF
- [UEFI loader](../lib/uefi/README.md) - `lib/uefi`, which loads and runs UEFI applications
  from LK: running it and its regression tests
- [GDB Debug Support for GBL](gdb_debug_support_for_gbl.md) - debugging GBL through the UEFI
  debug support protocol
- [Network Setup for QEMU](lk_tap.md) - tun/tap networking for QEMU runs

## Platforms and boards

- ARM Fixed Virtual Platform: [how to run](fvp-base/how_to_run.md) and
  [internal overview](fvp-base/internal.md) (boot sequence, memory map, peripherals)
- [ARM MPS2/MPS3](../platform/mps2/README.md) - the QEMU Cortex-M boards that cover every
  core generation from M3 to M55
- [SiFive HiFive Premier P550](../target/hifive-premier-p550/README.md)
- [Banana Pi BPI-F3](../target/bananapi-f3/README.md)

## Components

- [spifs](../lib/fs/spifs/README.md) - a small flat filesystem for SPI NOR flash

## Project

- [TODO list](todo.md) - open work, ideas and recently landed changes
- [GitHub repository](https://github.com/littlekernel/lk),
  [issues](https://github.com/littlekernel/lk/issues), and `#lk` on
  [Libera.Chat](https://libera.chat)
