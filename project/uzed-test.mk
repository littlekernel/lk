# top level project rules for the uzed-test project
#
MODULES += \
	app/inetsrv \
	app/lkboot \
	app/shell \
	dev/gpio \
	lib/klog \
	lib/watchdog \

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

include project/target/uzed.mk
include project/virtual/test.mk

