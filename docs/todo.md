# LK TODO

One line per item, jot new ones under **Unsorted** and sort later (or never).
Indent notes under an item. Optional tags: `[branch: x]` `[#123]` `[plan]` `[wip]`.
Strike an item with `~~` when done and move it to **Recently landed**; prune that section now and then.

Also: [github issues](https://github.com/littlekernel/lk/issues), [pull requests](https://github.com/littlekernel/lk/pulls).

## Unsorted

- 

## In flight (branches with real work)

- Break up thread_lock, per-cpu scheduler, preempt_disable [branch: preempt_disable] [#285]
    - complete, 45 commits, needs rebase (183 behind) + gate + merge
    - also pushed as origin/wip/preempt-disable
- Console output serialization, per-thread line buffers [branch: console-output-serialization] [#485]
    - compiles, never booted, 332 behind, conflicts in kernel/thread.c and top/debug.c
    - its console_ring_wake() poll loop waits on the preempt_disable work
- SD/MMC stack [branch: sdmmc]
    - merge + push; then board GPT check, card-detect hotplug, erase/TRIM, SDIO, CQE
- nRF54L15 + nrfx 3.14 roll [branch: nrfx-3.14-roll]

## Planned (plan written, no code)

- P550 PCIe root complex (DW core + eic7700 glue) [plan]
- P550 DesignWare GMAC driver [plan]
- virtio-fs FUSE client, no DAX [plan]
    - needs virtiofsd on the test hosts
- Shared page-table walker: traits/geometry split, then x86-64 port, then arm64 [plan]
    - x86-64 port also fixes: map over a live page succeeds, no ENOMEM rollback, destroy leaks tables
- Backtrace follow-ups: arm32/cortex-m walker, symtab prefix compression, DWARF CFI [plan]
- Rust on arm32 (incl cortex-m) and x86-32, then a rust riscv uart driver [plan]

## Stale WIP branches (revive or delete)

- x86 5-level paging [branch: la57] (1 WIP commit, 2025-11)
- GICv2m windows + GICv3 ITS discovery + msi-map from FDT [branch: origin/wip/msi] (2026-04)
- xhci skeleton [branch: origin/wip/usbhost] (2026-03)
- SPARC sun4m port, mmu started [branch: origin/wip/sparc] (2026-08)
- VAX port [branch: vax] (2019)
- stdatomic.h migration + atomic tests [branch: stdatomic] (2026-06)
    - scripts/check-gcc-atomic-builtins.sh is untracked and pairs with this
- pico SDK 2.1.0 roll [branch: origin/wip/pico-update]
- delete: unique-ptr-cleanup, save (both fully on master), pr-*-orig, pr-*-check
- archive or delete: lk-dynamic, rosco-m68k, riscv-smpwip, riscv-spi, riscv-cpp, build-llvm-cc,
  armi, avr32, debug, dpc, net

## Core kernel

- Ticket lock spinlock?
- arch_in_int_handler for all arches [#368]
    - only arm32 has one, via a global (not per-cpu); arm64 has none
    - then assert blocking apis aren't called in a handler (arch_blocking_disallowed a la fuchsia)
- No resched from irq context before EOI [#528]
- Priority inheritance for mutexes [#300]
- Concurrent thread_join [#477]
- port_write from irq context [#284]
- Loadable module support? (lib/elf loads static PT_LOAD only)
- atomic_cmpxchg() is a stub returning 0 (arch/include/arch/atomic.h)
- Atomic routines: all relaxed, add ordered variants [#277]
- Use AutoInterruptSave somewhere or drop it (exists, zero users) [#273]

## SMP / VMM

- x86: free emptied page tables *after* the tlb shootdown, not before (arch/x86/64/mmu.c)
- x86: shootdown should use mp_sync_exec_cpus like riscv
- x86-32: kernel page directory entries above the first 1GB never reach existing user aspaces
- Align arch_mmu_map over an existing mapping across arches [#356]
    - x86-64 and arm32 overwrite and return success; riscv and arm64 refuse
- vmm_map_physical(), make vmm_alloc_physical closer to sival
- Simple VMO object layer
- Mismatched cache attribute aliases of the same memory [#529]
- Remove USER_ASPACE_BASE/SIZE defaults from vm.h (arm32 and m68k still rely on them)
- Docs: physmap, what paddr_to_kvaddr returns, cached vs uncached

## X86 / PC

- 5 level paging [branch: la57]
- FRED?
- PCID is detected but never enabled
- mtrr.c has its own tlb flush, use the common one
- stale "TODO: proper tlb flush" comments in 32/mmu.c and 64/mmu.c
- platform_allocate_interrupts panics for count != 1 (MSI multi-vector)
- Clean up the framebuffer console from multiboot2 (own 9x16 font, 32bpp only), use lib/gfxconsole?

## ARM64

- Feature detection (read the ID registers, mirror x86/riscv feature.c)
- arm64_elX_to_el1: stay in VHE if already in it? SCR_EL3.NS? CNTHCTL_EL2 / ICC_SRE_EL2?
- Chainload support [#526]
- WITH_KERNEL_VM=0 doesn't link; force it on and drop the #ifs

## RISC-V

- APLIC + IMSIC support
- CLIC support [#527]
- time.c: interval * rate is a 32-bit multiply, wraps above ~430ms at 10MHz (fires early)
- current_time() off by 2.4% at 32768Hz (divides by rate/1000)
- No way to keep the compiler from using FP registers in kernel code
- MSI: every platform returns ERR_NOT_SUPPORTED
- sifive_u qemu timers [#369]
- RP2350 riscv (hazard3) bring-up
- visionfive2: add a dts + build script; jh7110 reset reg hardcoded; PCI from FDT is #if 0

## Devices

- Move remaining uarts into dev/
    - private 16550s: alterasoc, mediatek mt6735/mt6797, pc/debug.c, qemu-mips, bcm28xx miniuart
    - pc needs a port-I/O accessor in dwc8250; platform/pc/uart.c is #if 0, delete it
    - private pl011: bcm28xx, rp23xx (via pico sdk)
    - goldfish_tty (platform-local in qemu-virt-m68k)
- Delete dev/driver.c (orphan, includes the removed dev/driver.h)
- MSI / MSI-X
    - pci allocate_msi/msix: single vector only, leaks on error
    - qemu-virt-arm hardcodes the GICv2m doorbell, route through the gic driver
    - GICv3 ITS / LPIs [branch: origin/wip/msi]
- USB host (xhci) [branch: origin/wip/usbhost]
- USB client: more classes? (only cdcserial + bulktest) hid, mass storage, ecm
- Virtio: handle partial transfers (block ignores the used ring len)
- Virtio-block: "XXX not cache safe"; sg descriptor alloc failure only asserted
- Virtio: parse SHM regions (needed for virtio-fs DAX someday)
- m68k mmu is 68040 only, rules.mk still selects 68030/68060 [PR #533]

## lib

- lib/bio
    - 64bit block numbers (bnum_t is 32-bit, 2TiB cap)
    - flush and discard hooks (virtio-block already defines the feature bits)
    - range clamping wraps: offset+len (LP64) and block+count
    - async read/write of a zero-length range never calls the callback
- lib/partition
    - extended / logical partitions (EBR walk)
    - scan at probe (only `bio partscan` calls partition_publish today)
    - virtio-block has a stale lib/partition include + dep
    - expose GPT name/type guid; 4K sector devices untested
- Binary search tree from trusty [#297]
- bits.h: 1UL masks break for bit >= 32 on 32-bit; clz/ctz/ffs macro names collide
- ROUNDUP/ROUNDDOWN truncate 64-bit values when PAGE_SIZE is 1U<<n (arm, mips, m68k)
    - reference: zircon's align.h, https://fuchsia.googlesource.com/fuchsia/+/refs/heads/main/zircon/system/ulib/zircon-internal/include/lib/zircon-internal/align.h
- Add stdckdint.h
- INCBIN: users must list the files in MODULE_SRCDEPS by hand (fat test lists 2 of 4)

## Filesystems

- FAT: shared cluster extend/shrink helper so dir code can reuse it (file.cpp TODO)
- FAT: remove of a cached file under a different case spelling returns ERR_BUSY
- 9p: v9fs unit test always skips in CI (no -f); v9p_tests is console only
- 9p: optimize buffer creation per transaction
- ext2 tests not in CI
- ext4 read support [PR #303]

## Networking (minip)

- socket-style udp receive (udp_listen is callback-only; dup check / insert race)
- tx checksum offload (rx flags exist; tcp.cpp has `|| true`)
- per-interface dhcp vs static hook (netif_registration_callback "TODO: make this overridable")
- the three open TCP correctness gaps from the rework plan
- ipv6 [#289]

## Build system

- Prefix toolchain vars ARCH_CC etc (engine.mk TODO); there is no CXX or AR
- arch/x86/rules.mk cc-option probe runs before CC is set, x86-32 loses -fno-stack-protector
- gcc LTO ($(error) today); arm-m LTO broken
- riscv: no -mgeneral-regs-only equivalent; arm32 relies on the default soft abi
- .cpp -> .cc? (94 .cpp files, .cc rules already exist)
- clang CI never sets WERROR

## Github / CI

- x86-64-uefi boot test runs in no workflow (needs ovmf in CI); also x86-i440fx, x86-64-i440fx, riscv64-nosvvptc
- Fold scripts/uefi_unittest.exp (arm64 uefi_load test, clang workflow only) into the regular boot test run
- run-ext2-tests.py not in CI
- Reproducible builds [#447]
- PGO evaluation [#387]

## Testing

- lib/fixed_point/rules.mk has `MOUDLE_OPTIONS` typo, its tests never build
- lib/norfs/test unreachable
- lib/libc/test/a.out is a tracked host binary, delete
- fibo and mem_test could be unit tests; app/stringtests has no libc unittest; disktest console only
- port_tests two_threads_basic flakes ~1/8 on riscv64 4-cpu (lost wakeup?)

## Docs

- DEBUG_ASSERT / ASSERT / dprintf levels / LK_DEBUGLEVEL blurb
- vmm_overview.md has a stale paddr_to_kvaddr snippet
- stale text: workflow comments "DEBUG=0 has no console"; run-qemu-boot-tests --timeout help says 30s

## Platforms / ports

- Find an M55 board (mps3 an547 under qemu covers the core)
- VAX / SPARC: roll into mainline or not?
- Cortex-R5 [#516], msm8916 [#405]

## Quickie

- delete dev/driver.c
- delete lib/libc/test/a.out
- fix MOUDLE_OPTIONS typo in lib/fixed_point/rules.mk
- drop stale lib/partition include + dep from dev/virtio/block
- arm64 WITH_KERNEL_VM ?= 1  ->  := 1
- fix the two stale "DEBUG=0 has no console" workflow comments and the 30s help text
- check-gcc-atomic-builtins.sh: drop or1k, probe a v6-M target, then commit or delete it

## Recently landed

- ~~backtraces with symbols (lib/backtrace, lib/symtab)~~
- ~~minip rework (pktbuf, netstack thread, tcp queues, dns)~~
- ~~GPT partitions (lib/partition, not lib/ptable) + bio partdump~~
- ~~AHCI + async bio~~
- ~~x86 tlb shootdown; generic IPI cross calls (mp_sync_exec, mp_sync_exec_cpus)~~
- ~~arm64 percpu struct + cycle counter~~
- ~~remove the old DRIVER/DEVICE class model~~
- ~~remove or1k, microblaze~~
- ~~read/write FAT incl. dir growth; m68k 68040 mmu~~
- ~~rust for arm64 / x86-64 / riscv32 / riscv64~~
- ~~do-qemu bash arrays; buildall -u; run-qemu-boot-tests --ubsan + UBSAN workflow~~
- ~~FAT image tests in CI; kernel/init.h; KERNEL_ASPACE_* defaults gone~~
- ~~C++ RAII interrupt guard (AutoInterruptSave)~~
- ~~PC platform_halt reboot/shutdown hooks~~
- ~~x86 on-demand fpu context switching (lazy save removed, always eager now)~~
- ~~HiFive Premier P550; pico2 ARM side; dwc8250 shared uart~~
- ~~clang + LTO; V=1; MODULE_OPTIONS test; actions/* on current majors~~
- ~~user aspaces on arm64/riscv; riscv range walker + large pages + Svvptc~~
- ~~cherry pick ubsan fixes from PR 412~~
