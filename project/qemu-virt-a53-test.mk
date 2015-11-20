# main project for qemu-aarch64
ARCH := arm64
ARM_CPU := cortex-a53

MODULES += \
	app/shell

WITH_LINKER_GC := 0

include project/virtual/test.mk
include project/virtual/fs.mk
include project/virtual/minip.mk
include project/target/qemu-virt.mk

