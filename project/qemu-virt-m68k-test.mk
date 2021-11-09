# main project for qemu-riscv64
MODULES += \
	app/shell

include project/virtual/test.mk
include project/virtual/fs.mk
include project/virtual/minip.mk
include project/target/qemu-virt-m68k.mk

