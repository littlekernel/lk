# top level project rules for the zybo-test project
#
MODULES += \
	app/inetsrv \
	app/shell \
	app/stringtests \
	app/tests \
	app/zynq-common \
	app/lkboot \
	dev/gpio \
	lib/cksum \
	lib/debugcommands \
	lib/libm \

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

include project/target/zybo.mk

