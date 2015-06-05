# main project for qemu-arm
TARGET := vexpress-a9

MODULES += \
	app/shell \
	lib/evlog

GLOBAL_DEFINES += WITH_KERNEL_EVLOG=1

WITH_LINKER_GC := 0

include project/virtual/test.mk

