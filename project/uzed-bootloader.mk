# top level project rules for the uzed-bootloader project
#
MODULES += \
	app/shell \
	app/zynq-common \
	app/lkboot

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

include project/target/uzed.mk

