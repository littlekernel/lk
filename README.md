# The Little Kernel Embedded Operating System

The LK kernel is an SMP-aware kernel designed for small systems ported to a variety of platforms and cpu architectures.

See https://github.com/littlekernel/lk for the latest version.

### High Level Features

- Fully-reentrant multi-threaded preemptive kernel
- Portable to many 32 and 64 bit architectures
- Support for wide variety of embedded and larger platforms
- Powerful modular build system
- Large number of utility components selectable at build time

### Supported architectures

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

### [TODO](docs/todo.md)

### To build and test for ARM64 on linux

1. install or build qemu. v2.4 and above is recommended.
2. install gcc for arm64 (see note 1)
3. run scripts/do-qemuarm -6  (from the lk directory)
4. you should see 'welcome to lk/MP'

This will get you a interactive prompt into LK which is running in qemu
arm64 machine 'virt' emulation. type 'help' for commands.

Note: for ubuntu x86-64
sudo apt-get install gcc-aarch64-linux-gnu
or fetch a prebuilt toolchain from
https://newos.org/toolchains/aarch64-elf-14.1.0-Linux-x86_64.tar.xz


### Building with LLVM-based toolchains

To build LK with a LLVM-based toolchain you will have to manually specify the compiler and linker in the environemnt.
Unlike GCC clang is a cross-compiler by default, so the target needs to be specified as part of the CC/CXX/CPP variables.
For example, assuming LLVM is in `/opt/llvm/bin/`, the following command will work to build for 64-bit RISC-V:

```
gmake qemu-virt-riscv64-test 'CC=/opt/llvm/bin/clang --target=riscv64-unknown-elf' 'CPP=/opt/llvm/bin/clang-cpp --target=riscv64-unknown-elf' 'CXX=/opt/llvm/bin/clang++ --target=riscv64-unknown-elf' 'LD=/opt/llvm/bin/ld.lld' TOOLCHAIN_PREFIX=/opt/llvm/bin/llvm- CPPFILT=/opt/llvm/bin/llvm-cxxfilt
```
TOOLCHAIN_PREFIX can be set to use the LLVM binutils, but due to the different naming of `llvm-cxxfilt` vs `c++filt` it needs to be set explicitly.

To build for AArch64 the command looks similar, just with a different `--target=` flag.
```
gmake qemu-virt-arm64-test 'CC=/opt/llvm/bin/clang --target=aarch64-unknown-elf' 'CPP=/opt/llvm/bin/clang-cpp --target=aarch64-unknown-elf' 'CXX=/opt/llvm/bin/clang++ --target=aarch64-unknown-elf' 'LD=/opt/llvm/bin/ld.lld' TOOLCHAIN_PREFIX=/opt/llvm/bin/llvm- CPPFILT=/opt/llvm/bin/llvm-cxxfilt
```
