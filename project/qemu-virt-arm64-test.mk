# main project for qemu-aarch64
MODULES += \
	app/shell \
    lib/uefi \

MODULE_DEPS := \
	lib/rust_support

include project/virtual/test.mk
include project/virtual/fs.mk
include project/virtual/minip.mk
include project/target/qemu-virt-arm64.mk

