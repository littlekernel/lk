# LK project overview

- Project: Little Kernel (LK), a small SMP-aware embedded OS kernel used in embedded systems and bootloaders.
- Primary languages: C and assembly; limited C++14 without STL/exceptions/RTTI.
- Scope: portable kernel across many architectures (ARM32/ARM64/RISC-V/x86/M68K/Microblaze/MIPS/OpenRISC).
- Build model: hierarchical Project -> Target -> Platform -> Architecture plus module-based `rules.mk` files.
- Notable subsystems: scheduler/threading (`kernel/thread.c`), VM (`kernel/vm` for MMU builds), libc, platform bring-up.

## Repository structure (high level)
- `project/`: top-level build compositions.
- `target/`: board-specific settings and memory map constants.
- `platform/`: SoC/platform initialization and drivers.
- `arch/`: CPU/ISA-specific low-level code.
- `kernel/`: core kernel mechanisms.
- `lib/`, `dev/`, `app/`: libraries, device code, optional applications.
- `scripts/`: build+QEMU helpers and test launch scripts.
