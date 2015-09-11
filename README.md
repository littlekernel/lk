# lk

The LK embedded kernel. An SMP-aware kernel designed for small systems.

See travisg/lk for the latest version.

## build and test for ARM on linux

1. install qemu
2. install gcc for embedded arm (see note 1)
3. run scripts/do-qemu  (from the lk directory)
4. you should see 'welcome to lk/MP'

This will get you a interactive prompt into LK which is running in qemu
arm vexpress_a9 emulation. type 'help' for commands.

To quit you might need to kill <quemu-pid>.

note 1: for ubuntu this seem to work:
sudo apt-get install gcc-arm-none-eabi
