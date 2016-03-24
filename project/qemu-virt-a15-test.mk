# main project for qemu-arm32
MODULES += \
	app/shell

include project/virtual/test.mk
include project/virtual/fs.mk
include project/virtual/minip.mk
include project/target/qemu-virt-a15.mk

