# main project for qemu-riscv64
MODULES += \
	app/shell
SUBARCH := 64

include project/virtual/test.mk
include project/virtual/fs.mk
include project/virtual/minip.mk
include project/target/qemu-virt-riscv.mk

