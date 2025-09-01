# main project for qemu-aarch64
MODULES += \
	app/shell \
    lib/uefi \

ARM64_PAGE_SIZE := 65536

include project/virtual/test.mk
include project/virtual/fs.mk
include project/virtual/minip.mk
include project/target/qemu-virt-arm64.mk

