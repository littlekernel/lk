# FVP Base platform overview

### Boot sequence 

FVP Base emulator is typically launched using both a secure flashloader and
a normal flashloader. The configuration looks like this:

```shell
-C bp.secureflashloader.fname=path/to/arm-trusted-firmware/build/fvp/debug/bl1.bin \
-C bp.flashloader0.fname=path/to/fip.bin
```

We use Arm Trusted Firmware (ATF) as the secure monitor running at S-EL3. It
initializes the system, sets up the secure world, and pass control to the
non-secure world.

#### The boot sequence works in the following way:

- BL1 (Secure ROM Bootloader) - Initializes trusted components and loads the
  next stage (BL2).

- BL2 (Trusted Boot Firmware) - Verifies and loads the next stages: BL31 and
  BL33.

- BL31 (EL3 Runtime Firmware) - Provides runtime services at EL3, including
  PSCI (Power State Coordination Interface) and context switching between
  secure and non-secure worlds.

- BL32 (Secure Payload) - optional secure-world component that runs at S-EL1.
  It is used to provide Trusted Execution Environment (TEE). If not required,
  this stage can be skipped, and BL31 will directly pass control to BL33. In our
  case we do not use BL32.

- BL33 (Non-secure Payload) - The final stage of the boot chain, typically the
  entry point to the non-secure kernel or bootloader. We use little kernel as
  BL33.

#### Memory Usage (bytes) [RAM]

| Component |   Start   |   Limit   |  Size   |  Free   | Total  |
|-----------|-----------|-----------|---------|---------|--------|
| BL1       | 0x4034000 | 0x4040000 | 0x7000  | 0x5000  | 0xc000 |
| BL2       | 0x4020000 | 0x4034000 | 0x10000 | 0x4000  | 0x14000|
| BL2U      | 0x4020000 | 0x4034000 | 0xa000  | 0xa000  | 0x14000|
| BL31      | 0x4003000 | 0x4040000 | 0x22000 | 0x1b000 | 0x3d000|

#### Memory Usage (bytes) [ROM]

| Component |  Start    |  Limit    |  Size   |   Free    |  Total   |
|-----------|-----------|-----------|---------|-----------|----------|
| BL1       | 0x0000000 | 0x4000000 | 0x8bd0  | 0x3ff7430 | 0x4000000|

To check the memory layout, refer to [TF-A Memory Layout Tool
Documentation](https://trustedfirmware-a.readthedocs.io/en/latest/tools/memory-layout-tool.html)

#### Little kernel and DTB location

On the FVP platform, DTB start address and the BL33 entry point are both
configured in the platform’s source files:

- **DTB base address**: `0x82000000 (32MB size)`
- **BL33 entry address**: `0x88000000`

You can find these settings in the `platform_def.h` header file:

- [DTB
  address](https://github.com/ARM-software/arm-trusted-firmware/blob/v2.12.0/plat/arm/board/fvp/include/platform_def.h#L94-L100)
- [BL33 entry
  address](https://github.com/ARM-software/arm-trusted-firmware/blob/v2.12.0/plat/arm/board/fvp/include/platform_def.h#L143)


DTS for FVP Base you can find at [fdts](
https://github.com/ARM-software/arm-trusted-firmware/tree/v2.12.0/fdts)

### Peripheral Address Map

The peripheral address map for the FVP Base platform can be found in the following documentation:  
[Base Platform memory map](https://developer.arm.com/documentation/100964/latest/Base-Platform/Base-Platform-memory/Base-Platform-memory-map?lang=en)

Currently, little kernel supports:

- **UART0,PL011**  
- **GICv2**

### Interrupt Assignment

The interrupt assignment for the FVP Base platform can be found in the following documentation:  
[Base Platform interrupt assignments](https://developer.arm.com/documentation/100964/1128/Base-Platform/Base-Platform-interrupt-assignments?lang=en)
