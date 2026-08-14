# Booting LK via UEFI (x86-64)

The x86-64 pc build produces a kernel image that boots two ways from the same
binary:

- **Multiboot 1/2** — GRUB, QEMU `-kernel`, anything that speaks multiboot,
  exactly as before.
- **UEFI** — the flat binary carries a hand-written PE/COFF header (Linux
  EFI-stub style), so `lk.bin` *is* a valid UEFI application. The build also
  emits it as `build-<project>/lk.efi`; the two files are identical.

## How it works

The pieces:

- `arch/x86/64/efi-header.S` owns the first 4KB of the image: an `MZ` magic at
  offset 0, the PE/COFF header, and the multiboot 1/2 headers (which loaders
  find by scanning the first 8KB, so they coexist with the PE header). All PE
  size/offset fields are computed at link time in `arch/x86/64/kernel.ld` —
  no post-processing tool is involved, and the reference edk2 loader does not
  validate the PE checksum.
- `arch/x86/64/efi/` is the in-kernel boot stub, entered at `efi_entry` (the PE
  entry point) with firmware paging live, at whatever address the firmware
  chose. It is compiled `-fpie` and is fully self-contained; the linker
  asserts it generated no GOT entries. It:
  1. converts `LoadOptions` (UCS-2) to an ASCII command line,
  2. records the GOP framebuffer, the ACPI RSDP from the EFI configuration
     table, and the system table pointer,
  3. reserves the kernel's link-time physical range (2MB) with
     `AllocatePages`, failing gracefully back to the firmware if it is not
     free,
  4. captures the memory map and calls `ExitBootServices` (with the spec's
     one-retry map-key dance),
  5. copies the image to its link address, drops from long mode back to
     32-bit protected mode, and jumps into the regular multiboot entry path
     (`real_start`) with no multiboot magic.

  Everything recorded lands in `struct lk_efi_boot_info`
  (`top/include/lk/efi_boot_info.h`), a `.data` resident structure that
  travels with the image copy.
- `platform/pc` checks `lk_efi_boot_info` before the multiboot info: the EFI
  memory map (coalesced, since EFI maps are heavily fragmented) feeds the same
  pmm arena setup, the command line feeds `lib/cmdline`, the GOP framebuffer
  feeds the fb console, and uACPI takes the RSDP from the boot info instead of
  scanning the BIOS area (which does not exist on pure UEFI machines).

The firmware may load the image anywhere, including above 4GB: the stub is
position independent, and the long-mode exit only executes from the copy at
its link address, which is always below 4GB.

## Running under QEMU + OVMF

`scripts/do-qemux86 -u` builds and boots through OVMF (install the
`ovmf`/`edk2-ovmf` package):

```bash
# boot to the shell
scripts/do-qemux86 -u

# with a command line: staged via the EFI shell's startup.nsh, since a
# default EFI/BOOT/BOOTX64.EFI boot gets empty LoadOptions
scripts/do-qemux86 -u -s 4 -A 'lk.autorun=ut all; poweroff'

# the unit test suite has a UEFI variant
./scripts/run-qemu-boot-tests.py --arch x86-64-uefi
```

The script synthesizes the ESP from `build-<project>/esp/` using QEMU's
`fat:rw:` virtual FAT drive — no disk image or mtools needed.

For real hardware, place `lk.efi` at `\EFI\BOOT\BOOTX64.EFI` on the ESP, or
add a boot entry with `efibootmgr`, or serve `lk.bin`/`lk.efi` directly as
the UEFI PXE boot file (the firmware's network loader takes any PE image; no
intermediate loader needed). A command line only reaches the kernel when the
boot entry carries load options (or when launched from the EFI shell).

## Limitations / future work

- GRUB-on-UEFI via multiboot2 still uses the legacy i386 handoff and is not
  supported; that would need the MB2 EFI boot-services and EFI amd64 entry
  tags (7/9). Direct UEFI boot covers most of those use cases.
- EFI runtime services are not used after `ExitBootServices` (no
  `SetVirtualAddressMap`); the system table address is stashed in
  `efi64_system_table` for future use.
- The stub requires the kernel's 2MB physical link address to be free, which
  holds on every firmware seen so far; a relocatable kernel would remove the
  requirement.
- arm64/riscv: the boot-info structure and stub logic are deliberately
  arch-neutral. arm64 needs the Linux-style trick of making the first
  instruction double as the `MZ` magic (and relocating the PSCI secondary
  entry, which currently sits at image offset 4), plus an MMU-off teardown;
  riscv is easier (firmware hands off with the MMU off, and the early code is
  already PC-relative) but needs `a0/a1` disambiguation and the
  `RISCV_EFI_BOOT_PROTOCOL` for the boot hart id.
