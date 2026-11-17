# Legacy PCs (386 and 486)

`pc-x86-legacy-test` builds the PC platform with `CPU=legacy`, for machines from before the
Pentium and PCI: a 386 (with or without a 387) or a 486. Nothing on such a machine can be
enumerated, so the platform does what the original AT software did: it reads what the BIOS
recorded and probes the well known ports to see what really answers.

## What the configuration is

- `-march=i386 -Os`, `X86_LEGACY=1`, one cpu (`WITH_SMP=0`).
- Interrupts on the 8259 pair only; no ACPI, IOAPIC, local APIC or HPET.
- The PIT is the only clock, at 1ms resolution. There is no TSC use.
- 4KB pages only, and a 16MB physmap (`PHYSMAP_SIZE`), so memory above 16MB is ignored.
- No PCI. A PCI 486 is driven as an ISA machine; its compatibility mode IDE sits at the ISA
  addresses anyway.
- The legacy-only drivers below (`dev/block/ide`, `dev/block/floppy`, `dev/bus/isa`) are
  built in this configuration and no other.

### CPU handling

- 386 vs 486 comes from whether EFLAGS.AC can be set, and cpuid from EFLAGS.ID. Without
  cpuid a leaf 1 is synthesized, with the FPU bit from an `fninit` probe. The boot log
  says `X86: no cpuid, x87 present` (or `not present`).
- A 386 has no `invlpg`, so its TLB is flushed with a cr3 reload. CR0.WP is only set on a
  486 or later.
- The x87, if any, is shared by every thread: there is no per thread x87 state, so x87
  code (libm, `bench`) must only run on one thread at a time.
  - A 486 reports x87 errors as #MF (CR0.NE).
  - A 386 with a 387 has no CR0.NE, so its errors arrive on IRQ13. The handler releases
    the coprocessor's BUSY# latch through port 0xf0 and panics with the status word.
  - With no coprocessor CR0.EM is set, and any x87 instruction is a fatal #NM.
- `x87 info` prints the setup, and `x87 fault` raises a divide by zero on purpose.

## What is probed

`pc_legacy_probe()` in `platform/pc/legacy.c` runs from `platform_init()`. The BIOS data
area and the CMOS are only hints, which get reported; a device counts as found when its
own probe answers.

| Device | Where | Probe |
|---|---|---|
| Serial | COM1-4 at 0x3f8, 0x2f8, 0x3e8, 0x2e8, or the BIOS's own address if it lists another | interrupt enable register, scratch register and FIFO bits: 8250, 16450, 16550, 16550A |
| IDE | 0x1f0/0x3f6 irq 14, 0x170/0x376 irq 15 | status not floating, sector registers hold a pattern |
| Floppy controller | 0x3f0, irq 6, DMA channel 2 | VERSION command; the drives come from CMOS 0x10 |

The boot log prints a `LEGACY:` line for each device found. The `legacy` console command
prints the raw BIOS data area and CMOS fields next to the same report. It is the first
thing to run on a new machine.

When the loader passes no memory map at all, memory above 1MB is sized from CMOS
0x30/0x31, the extended memory POST found.

### Serial console

The console starts on `DEBUG_COM_PORT` (COM1 unless the build says otherwise) at 115200.
`pc.console=com1` through `com4` on the command line moves it once the command line has
been read. A port that doesn't answer is refused with a message. With no uart at all the
console's serial side is off, and its receive irq is never enabled.

The rest of the PC platform shares this serial code: the probe runs on modern builds too.

### IDE

- PIO only, LBA48, LBA28 or CHS.
- Drives without LBA (IDENTIFY word 49 bit 9) get INITIALIZE DEVICE PARAMETERS with their
  default geometry and are addressed by cylinder/head/sector. `ide.force_chs` on the
  command line does the same to any drive, which is how the CHS path gets tested on QEMU.
- Each sector waits for the drive's interrupt. If a channel's interrupt never arrives
  (IRQ14/15 not wired, or jumpered elsewhere) but the drive finished, the channel
  switches to polling and logs it once.
- Disks register with bio as `ide<channel>.<drive>`. There is no ATAPI support.

### Floppy

- Read only. Drives register with bio as `fd0` and `fd1`, with the type the CMOS gives.
- A disk's density is found on the first access after a disk change, by trying READ ID at
  each data rate the drive allows:
  - 1.44M drive: 1.44M, then 720K
  - 2.88M drive: 2.88M, 1.44M, then 720K
  - 1.2M drive: 1.2M, then 360K (double stepped, 300kbps)
  - 720K and 360K drives: their own format
- The bio device's size follows the disk, so it is the drive's native size until a disk
  has been read.
- A FAT floppy mounts read only with `mount /fd fat fd0 ro`.
- A drive with no disk is refused through the change line, except on 360K drives, which
  have none.

## Running under QEMU

```bash
scripts/do-qemux86 -l                       # 486, -machine isapc, 4MB unless -m says otherwise
scripts/do-qemux86 -l -m 16 -d disk.img     # ISA IDE disk
scripts/do-qemux86 -l -m 16 -y floppy.img   # floppy in drive A
./scripts/run-qemu-boot-tests.py --arch x86-legacy
```

`scripts/mkpattern-image.py` writes an image in which every 512 byte block names itself.
The `bio_pattern` unit test checks any device carrying that stamp, which catches
addressing bugs a plain round trip would miss:

```bash
scripts/mkpattern-image.py disk.img --size 20M
scripts/do-qemux86 -l -m 16 -d disk.img -A ide.force_chs -A 'lk.autorun=ut+bio_pattern'
scripts/mkpattern-image.py fd.img --floppy 1.44M
scripts/do-qemux86 -l -m 16 -y fd.img -A 'lk.autorun=ut+bio_pattern'
```

The test writes to the image and restores it, so use a scratch copy. QEMU drive types for
the floppy are set with `-- -global isa-fdc.fdtypeA=120` (or 144, 288).

`./scripts/run-fat-tests.py --arch x86-legacy` runs the FAT image tests on the ISA IDE disk,
which also exercises its write path.

### What QEMU cannot show

- There is no 386 model, and QEMU's 486 always has cpuid and an FPU. The 386 detection, the
  no-cpuid path, the TLB flush without `invlpg`, IRQ13 error reporting and the no-FPU
  path only run on real hardware.
- QEMU's drives always support LBA. `ide.force_chs` exercises the CHS arithmetic, but not a
  drive that really lacks LBA.
- QEMU models a 1.2M drive holding a 360K disk with 40 physical tracks, so the double
  stepped seek fails there. Real 1.2M drives have 80.
- QEMU leaves the CMOS checksum unset, so `legacy` reports it as BAD.
- `poweroff` does not exit QEMU on isapc, which has no ACPI power management port.

## Real hardware

The kernel is a multiboot image (`lk.bin`, or `lk.elf`). A GRUB legacy boot floppy that
loads it with `kernel /lk.bin.gz` boots both a 386 and a 486. It loads at 2MB physical
and ends a little under 3MB. `ut all` passes in 4MB; below 8MB some of the FAT tests
skip the RAM volumes they can't allocate.

For the first bring-up on a machine:

1. Boot it. Note the `X86:` cpu and x87 lines, the memory map, and the `legacy` report.
2. Run `ut all`, `x87 info` and `bench`. On a 386 also try `x87 fault`, which should panic
   through IRQ13.
3. Read from each IDE disk (`bio read`, or `ut bio_pattern` on a stamped spare disk), and
   from a floppy in each drive. Only write to a disk whose contents don't matter.
