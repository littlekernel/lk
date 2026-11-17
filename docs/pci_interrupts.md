# PCI interrupts: INTx and MSI

How a PCI driver's interrupt finds its way to a vector, for legacy INTx pins and for MSI/MSI-X,
on device tree platforms (qemu-virt-arm, qemu-virt-riscv and other fdtwalk users) and on the PC.

## The driver side

Drivers ask the bus manager (`dev/bus/pci/include/dev/bus/pci.h`) and fall back in order:

```c
uint vector;
if (pci_bus_mgr_allocate_msix(loc, 1, &vector) != NO_ERROR &&
    pci_bus_mgr_allocate_msi(loc, 1, &vector) != NO_ERROR) {
    pci_bus_mgr_allocate_irq(loc, &vector);   // INTx
}
register_int_handler_msi(vector, handler, arg, true);   // or register_int_handler for INTx
unmask_interrupt(vector);
```

Each call returns a platform interrupt vector, so the rest of the driver does not care which
kind it got. MSI and MSI-X hand out one vector per function today.

## Legacy INTx

`device::allocate_irq` (`dev/bus/pci/bus_mgr/device.cpp`) builds the path from the root bus
down to the function (each bridge's device/function, then the function itself) and asks the
root's `intx_route` hook. Without a hook, or if it fails, it falls back to the interrupt line
register and then to the platform's `platform_pci_int_pin_to_vector`.

- **Device tree.** `fdtwalk_setup_pci` gives a root whose pcie node has an `interrupt-map` an
  `intx_route` hook. The hook swizzles the pin up through every bridge on the path (PCI-PCI
  bridge spec: pin `p` on a bridge's secondary bus arrives as `((p - 1 + device) % 4) + 1`), then
  looks up the root bus slot in the masked map. The parent specifier is decoded when the parent
  is a GIC (SPI + 32, PPI + 16) or a single cell controller such as the riscv PLIC. The qemu-virt
  platforms' own swizzle is only the fallback for a tree without a map, and only right on the
  root bus.
- **PC.** `platform/pc/pci.c` routes through the ACPI `_PRT`.

arm_gic and riscv_plic keep one handler per vector, so two functions whose pins land on one line
cannot share it: the second registration replaces the first and prints a warning.

## MSI and MSI-X

`device::allocate_msi`/`allocate_msix` get a vector and the message that raises it (an address
and the data to write there) from the root's `msi_ops` hook when the root has one, and from
`platform_allocate_interrupts`/`platform_compute_msi_values` otherwise (the PC's local APIC).

On device tree platforms with a GIC, `fdtwalk_setup_pci` gives a root whose pcie node has an
`msi-map` (or `msi-parent`) naming a GIC MSI frame an `msi_ops` hook. The hook translates the
function's requester id (`bus << 8 | dev << 3 | fn`) through the map into a frame and a device id,
then calls `arm_gic_msi_alloc()` (`dev/interrupt/arm_gic/include/dev/interrupt/arm_gic.h`). A frame
is named by its physical base. QEMU virt's map is the identity: device id = requester id.

### GICv2m

A GICv2m frame (`dev/interrupt/arm_gic/gic_v2m.c`) turns a write of an SPI number to its
`MSI_SETSPI_NS` register into that SPI. The SPIs it can raise come from its `MSI_TYPER`, clipped
to the platform's `MAX_INT`. QEMU virt has one frame beside a GICv2, raising SPIs 80..143.

### GICv3 ITS (arm64)

On a GICv3 the map names an ITS (`gic_v3_its.c`), which turns a device's write of an event id to
`GITS_TRANSLATER` into an LPI (`gic_v3_lpi.c`).

- **Setup.** LPIs and the first ITS are set up from `arm_gic_init_map` when the device tree has
  an ITS and the distributor reports LPI support. There is one LPI configuration table for every
  cpu and a pending table per cpu. Each ITS table's page size, shareability and size are read
  back and must match what it was sized for, and the enable must stick. A redistributor that
  cannot take LPIs, or already has them on, is left alone with a message.
- **Collections.** Each cpu maps its own collection (`MAPC`) once LPIs are on.
- **Allocation.** A device id gets an ITT with room for 32 events on its first MSI (`MAPD`). Each
  MSI is an LPI mapped from the next free event (`MAPTI`), targeting cpu 0's collection.
- **Masking.** Masking an LPI rewrites its configuration byte and sends `INV`.
- **Limits.** LK hands out 256 LPIs (8192..8447, `ARM_GIC_MAX_LPIS`), uses only the first ITS,
  and does not handle requester id aliasing behind a PCIe to PCI bridge.

QEMU and KVM skip a malformed ITS command without reporting it, so the driver checks ids against
its table sizes before queuing one; `-d guest_errors` on the QEMU command line shows any that
slip through.

## Testing

- `./scripts/run-qemu-boot-tests.py --arch arm64-gicv3` boots a 4 cpu GICv3 with the virtio
  devices on PCI, so they come up on MSI-X through the ITS.
- The `gic_its` unit test (`dev/interrupt/arm_gic/test`) raises LPIs with the ITS `INT` command
  and checks delivery, masking, and moving one to each cpu's collection. It skips without an ITS.
- For a live check, `scripts/do-qemuarm -6 -G 3 -C -n -A 'lk.autorun=sleep+8;mi+s;poweroff'`
  should show a DHCP lease on `virtio-net-0`. Under KVM, `-k` on an arm64 host with a GICv3 uses
  the kernel's ITS emulation, which forces 64K table pages and 8 byte ITT entries.
