# The Little Kernel Embedded Operating System

The LK kernel is an SMP-aware kernel designed for small systems ported to a variety of platforms and cpu architectures.

It is used in a variety of open source and closed source projects, notably the bootloader for a lot of Android phones of various make.

See https://github.com/littlekernel/lk for the latest version.

For comprehensive documentation, see [Index](docs/index.md).

## High Level Features

- Fully-reentrant multi-threaded preemptive kernel
- Portable to many 32 and 64 bit architectures
- Support for wide variety of embedded and larger platforms
- Powerful modular build system
- Large number of utility components selectable at build time

## Supported architectures

- ARM32
  - Cortex-M class cores (armv6m - armv8m)
  - ARMv7+ Cortex-A class cores
- ARM64
  - ARMv8 and ARMv9 cores
- RISC-V 32 and 64bit bit in machine and supervisor mode
- x86-32 and x86-64
- Motorola 68000
- Microblaze
- MIPS
- OpenRISC 1000
- VAX (experimental)
