# top level project rules for the zybo-test project
#
MODULES += \
	app/inetsrv \
	app/shell \
	app/lkboot \
	dev/gpio \

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

include project/target/zybo.mk
include project/virtual/test.mk

