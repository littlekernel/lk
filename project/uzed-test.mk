# top level project rules for the uzed-test project
#
MODULES += \
	app/inetsrv \
	app/shell \
	app/stringtests \
	app/tests \
	app/lkboot \
	app/zynq-common \
	dev/gpio \
	lib/cksum \
	lib/debugcommands \
	lib/libm \

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

include project/target/uzed.mk

