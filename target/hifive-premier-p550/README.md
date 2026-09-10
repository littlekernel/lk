# SiFive HiFive Premier P550 (HF106)

ESWIN EIC7700X SoC, 4x SiFive P550 rv64gc cores at 1.4GHz, 16 or 32GB LPDDR5.
LK runs in supervisor mode under the board's stock OpenSBI + U-Boot, which live in
SPI flash. Nothing on the board needs reflashing.

## Console, power and debug

The rear USB-C carries an FT4232H with four channels, which Linux enumerates in order:

| FTDI interface | Function |
|---|---|
| 0 | SoC JTAG (unbind ftdi_sio for OpenOCD) |
| 1 | board MCU JTAG |
| 2 | SoC UART0, 115200 8N1, the LK console |
| 3 | board MCU console, 115200 8N1 |

The board MCU (the docs call it the BMC) controls SoC power, so it is what to drive for
unattended reboots. Its CLI wants LF line endings, CR alone is ignored:

```
sompower-g          # Som Power Status: ON / OFF
sompower-s 1        # power the SoC on (0 = off)
reboot cold         # or warm
somconsole-g        # UART or Telnet; if Telnet, UART0 is routed to the MCU, not the USB
```

`somwork` and `temp` read the SoC through a Linux daemon, so under LK they report
stopped or fail; only the power commands matter here.

`poweroff` and `reboot` in LK go through SBI, and OpenSBI forwards them to the MCU over
UART2, so they really cut power and really reset.

OpenOCD: `sifiveinc/hifive-premier-p550-tools/jtag/openocd_mcpu.cfg` (ftdi driver,
channel 0, vid:pid 0403:6011, four riscv targets). Jumper J53 open routes JTAG to the
FT4232H; closed routes it to the 10 pin header.

## Booting from U-Boot

Build, wrap the flat binary in a legacy uImage and load it over TFTP. The load address
matches `KERNEL_LOAD_OFFSET` above OpenSBI and U-Boot's own `loadaddr`. The build
defaults to sv48 paging; `RISCV_MMU=sv39` on the make command line selects sv39, and the
same uImage recipe applies since the load address is physical.

```
make hifive-premier-p550-test
mkimage -A riscv -O linux -T kernel -C none -a 0x80200000 -e 0x80200000 \
        -n lk -d build-hifive-premier-p550-test/lk.bin lk.uimg
```

U-Boot only drives the MAC at 0x50400000 (`eth0`), which is the gigabit port farther from
the HDMI connector; the other one gets no link at the U-Boot prompt. Interrupt autoboot
on UART0 (two second delay), then:

```
dhcp
tftpboot ${kernel_addr_r} <server>:lk.uimg
bootm ${kernel_addr_r} - ${fdtcontroladdr}
```

U-Boot drops characters typed faster than it polls the UART, so paste slowly or use
`scripts/uboot-tftp-boot.py`, which does the whole sequence and logs the console:

```
scripts/uboot-tftp-boot.py /dev/ttyUSB3 boot.log <server> lk.uimg \
        --bootargs 'lk.autorun=ut+all;poweroff'
```

`bootm` copies the image to 0x80200000 and enters with `a0` = hart id and `a1` = the
device tree, which is where the memory layout and secondary harts come from. Setting
`/chosen/bootargs` in that tree is how a boot script reaches LK:

```
fdt addr ${fdtcontroladdr}
fdt set /chosen bootargs "lk.autorun=ut+all;poweroff"
```

## Boot select switch

SW1 as shipped is `ON,ON,OFF,ON` (BOOT_SEL 0b0010), boot from SPI flash. Leave it there.
