# LK

The LK embedded kernel. An SMP-aware kernel designed for small systems.

See https://github.com/littlekernel/lk for the latest version.

See https://github.com/littlekernel/lk/wiki for documentation.

## Builds

[![Build Status](https://travis-ci.org/littlekernel/lk.svg?branch=master)](https://travis-ci.org/littlekernel/lk)

## To build and test for ARM on linux

1. install or build qemu. v2.4 and above is recommended.
2. install gcc for embedded arm (see note 1)
3. run scripts/do-qemuarm  (from the lk directory)
4. you should see 'welcome to lk/MP'

This will get you a interactive prompt into LK which is running in qemu
arm machine 'virt' emulation. type 'help' for commands.

note 1: for ubuntu:
sudo apt-get install gcc-arm-none-eabi
or fetch a prebuilt toolchain from
http://newos.org/toolchains/arm-eabi-5.3.0-Linux-x86_64.tar.xz
