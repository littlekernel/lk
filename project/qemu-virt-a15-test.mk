# main project for qemu-aarch64
TARGET := qemu-virt
ARCH := arm
ARM_CPU := cortex-a15

MODULES += \
	app/shell

WITH_LINKER_GC := 0

include project/virtual/test.mk
include project/virtual/minip.mk

