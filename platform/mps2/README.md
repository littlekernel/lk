# ARM MPS2/MPS3 platform

Support for the Cortex-M FPGA images that qemu models for ARM's MPS2 and MPS3
prototyping boards. These exist mostly to give the `arch/arm/arm-m` code a set
of emulated targets covering every core generation, from armv7-m through
armv8-m and armv8.1-m.

One platform covers all of the machines; they differ only in the cpu, the
memory map, the uart base and interrupt numbers, and the clock rates. The
machine is selected with `MPS2_MACHINE` in the project's target fragment under
`project/target/`.

| project | qemu machine | cpu | notes |
|---|---|---|---|
| `mps2-an385-test` | `mps2-an385` | Cortex-M3 | |
| `mps2-an386-test` | `mps2-an386` | Cortex-M4 | with FPU |
| `mps2-an500-test` | `mps2-an500` | Cortex-M7 | with FPU |
| `mps2-an505-test` | `mps2-an505` | Cortex-M33 | armv8-m, IoTKit, with FPU |
| `mps3-an547-test` | `mps3-an547` | Cortex-M55 | armv8.1-m, SSE-300 |

Build and run one with the qemu wrapper script:

```
scripts/do-qemuarm -B an385
scripts/do-qemuarm -B an547 -A 'lk.autorun=ut+all;poweroff'
```

## Notes on the hardware

All of the machines use the same ARM CMSDK APB UART, and all of them boot by
fetching the initial stack pointer and reset vector from the bottom of the
image, so the read only half of the image lives at address 0 (the an505 is the
exception, see below).

Timekeeping is SysTick only, running off the core clock. The SSE based machines
(an505, an547) do not wire up a SysTick reference clock at all, and the an547's
own timers are a completely different device from the CMSDK timers on the other
boards, so the core clock is the one source that works everywhere.

The an505 and an547 are TrustZone parts, but nothing here is TrustZone aware.
The cpu comes out of reset secure with the SAU disabled, which leaves the whole
address space secure, so the kernel runs as an ordinary secure image and never
touches the SAU or the memory protection controllers.

The an505 is the one machine that does not come out of reset with VTOR at zero:
it points at `0x10000000`, the secure alias of the RAM at zero. qemu reads the
reset vector out of the loaded image by its link address, so that image has to
be linked at the alias rather than at zero, and `ROMBASE`/`MEMBASE` are set to
the secure aliases accordingly.

## Semihosting

These boards have no way to hand a command line to the guest and no power
controller, so the platform uses ARM semihosting for both: the kernel command
line is read from the host at boot, and `poweroff` asks the host to terminate.
This is what lets `lk.autorun` and the automated boot tests work.

That means qemu has to be started with `-semihosting-config enable=on`, which
`scripts/do-qemuarm` always does. To run under a bare qemu without it, build
with `MPS2_WITH_SEMIHOSTING=0`; otherwise the trap instruction faults with
nobody listening.
