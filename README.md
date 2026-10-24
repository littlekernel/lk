# LK — The Little Kernel

[![LK CI (gcc)](https://github.com/littlekernel/lk/actions/workflows/github-ci.yml/badge.svg)](https://github.com/littlekernel/lk/actions/workflows/github-ci.yml)
[![LK CI (Clang)](https://github.com/littlekernel/lk/actions/workflows/github-ci-clang.yml/badge.svg)](https://github.com/littlekernel/lk/actions/workflows/github-ci-clang.yml)
[![QEMU boot tests](https://github.com/littlekernel/lk/actions/workflows/qemu-boot-tests.yml/badge.svg)](https://github.com/littlekernel/lk/actions/workflows/qemu-boot-tests.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

LK is a small, SMP-aware embedded kernel that runs in supervisor mode on a wide range of
32 and 64-bit CPUs, from Cortex-M microcontrollers with a few KB of RAM to multi-core
ARM64, RISC-V and x86-64 systems with virtual memory. It is written in C and assembly, with
a curated subset of C++17 and optional Rust.

The canonical repository is <https://github.com/littlekernel/lk>.

## Where it's used

LK rarely appears on a spec sheet, but it runs underneath a great many devices, mostly as
the bootloader or secure-world OS beneath the system the user sees:

- **Android bootloaders.** Qualcomm's long-running `aboot` bootloader and MediaTek's
  second-stage bootloader are both built on LK
  ([Android boot process](https://en.wikipedia.org/wiki/Booting_process_of_Android_devices)).
  Android's [Generic Bootloader](https://source.android.com/docs/core/architecture/bootloader/generic-bootloader/gbl-dev)
  documentation lists LK as one of the supported host bootloaders.
- **Trusty TEE.** Android's trusted execution environment is
  ["a small OS kernel derived from Little Kernel"](https://source.android.com/security/trusty),
  with distributions from [Intel](https://www.intel.com/content/www/us/en/developer/topic-technology/open/trusty/overview.html)
  and [NVIDIA Jetson](https://docs.nvidia.com/jetson/archives/l4t-archived/l4t-3276/Tegra%20Linux%20Driver%20Package%20Development%20Guide/trusty.html).
- **NVIDIA TLK.** The Trusted Little Kernel secure OS for Tegra
  ([Trusted Firmware-A docs](https://trustedfirmware-a.readthedocs.io/en/latest/components/spd/tlk-dispatcher.html)).
- **Fuchsia.** "Zircon was born as a branch of LK"
  ([Zircon and LK](https://fuchsia.dev/fuchsia-src/concepts/kernel/zx_and_lk)).
- **Community bootloaders** such as [lk2nd](https://github.com/msm8916-mainline/lk2nd), which
  postmarketOS uses on Qualcomm phones and tablets.
- **Embedded products and hobby systems** that want a small threaded kernel without a full
  OS around it.

## Features

- Preemptive, priority-based, fully reentrant multithreaded kernel with SMP support
- Mutexes, semaphores, events, ports, timers and spinlocks
- Virtual memory with kernel and user address spaces on MMU architectures, physical-only
  operation on everything else
- Modular make-based build: pick a project, get exactly the modules it lists
- A large library of optional components:
  - filesystems: FAT, ext2, 9p, spifs, memfs, with GPT partition support
  - the `minip` TCP/IP stack, virtio, PCI/PCIe, ACPI, device tree
  - an interactive shell and console commands, a unit test framework
  - symbolic backtraces, UBSAN, a UEFI loader and boot stub
- Builds with GCC or Clang/LLD, with optional LTO

## Architectures

| Architecture | Cores / modes |
| ------------ | ------------- |
| ARM64        | ARMv8 and ARMv9, 4K/16K/64K pages |
| ARM32        | ARMv7+ Cortex-A, Cortex-M (ARMv6-M through ARMv8.1-M) |
| RISC-V       | RV32 and RV64 in machine mode; RV64 in supervisor mode with sv39 or sv48 paging |
| x86          | 32-bit and x86-64 |
| m68k         | 68000 through 68040 |
| MIPS         | MIPS32 (build only, not covered by the boot tests) |

Every architecture except MIPS is booted under QEMU in CI and runs the full unit test suite.

## Platforms

A sample of what is in [`platform/`](platform) and [`target/`](target):

- **Emulated:** QEMU `virt` for ARM, ARM64, RISC-V and m68k; QEMU PC (q35 and i440fx, BIOS
  or UEFI); the ARM MPS2/MPS3 Cortex-M boards; the ARM Fixed Virtual Platform
- **ARM boards:** Raspberry Pi 2/3, Raspberry Pi Pico and Pico 2, Xilinx Zynq, many STM32
  parts, Nordic nRF51/nRF52, NXP LPC, TI Stellaris, MediaTek and Amlogic SoCs
- **RISC-V boards:** SiFive HiFive, StarFive VisionFive 2, Banana Pi BPI-F3, HiFive Premier P550
- **m68k:** rosco-m68k, Amiga

Each buildable configuration is a file in [`project/`](project).

## Quick start

On a Linux or macOS host with QEMU installed:

```bash
git clone https://github.com/littlekernel/lk
cd lk

# Fetch a prebuilt cross toolchain into toolchain/ and put it on the PATH
scripts/fetch-toolchains.py --prefix aarch64-elf
export PATH=$PWD/$(ls -d toolchain/aarch64-elf-*/bin | tail -1):$PATH

# Build qemu-virt-arm64-test and boot it
scripts/do-qemuarm -6
```

You land at an LK shell prompt: try `help`, `threads`, or `ut all` to run the unit tests.
Exit QEMU with `Ctrl-A x`.

The other launchers follow the same pattern: `scripts/do-qemuriscv -6S` (RISC-V 64,
supervisor mode), `scripts/do-qemux86 -6` (x86-64), `scripts/do-qemum68k`. Each takes `-h`.
Distribution cross compilers such as `gcc-aarch64-linux-gnu` also work.

## Building

```bash
make qemu-virt-arm64-test                                  # output in build-qemu-virt-arm64-test/
make qemu-virt-arm64-test TOOLCHAIN=clang LD=ld.lld        # Clang + LLD
make qemu-virt-arm64-test TOOLCHAIN=clang LTO_MODE=thin    # with ThinLTO
make qemu-virt-arm64-test USE_RUST=1                       # link in the Rust crates
make qemu-virt-arm64-test DEBUG=0                          # release build
scripts/buildall -q -e                                     # every project, warnings as errors
```

Persistent settings go in a `local.mk` at the top of the tree. Tab completion for project
names is in `scripts/completion.sh`; source it from your shell's rc file.

## Testing

```bash
./scripts/run-qemu-boot-tests.py --arch arm64     # build, boot and run `ut all`
./scripts/run-qemu-boot-tests.py                  # every architecture in CI
```

Any shell command can run at boot via the `lk.autorun` command line variable, e.g.
`scripts/do-qemuarm -6 -A 'lk.autorun=ut all; poweroff'`.

## Documentation

- [Getting started](docs/getting_started.md), including setting up an out-of-tree project
- [Documentation index](docs/index.md): threads and scheduling, blocking primitives, the VMM,
  the filesystem layer, source tree layout, UEFI boot, QEMU networking
- [`AGENTS.md`](AGENTS.md): a dense guide to the build system, conventions and workflows,
  written for coding agents but useful to anyone

## Community

- Bugs and feature requests: [GitHub issues](https://github.com/littlekernel/lk/issues)
- Patches: [pull requests](https://github.com/littlekernel/lk/pulls) against `master`; commit
  messages use `[module][submodule] short description`
- Chat: `#lk` on [Libera.Chat](https://libera.chat)

## License

LK is released under the [MIT license](LICENSE). Some third-party code under `external/`
carries its own license.
