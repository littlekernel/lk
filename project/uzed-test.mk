# top level project rules for the uzed-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \
	app/inetsrv \
	app/shell \
	app/stringtests \
	app/tests \
	app/zynq-common \
	app/lkboot \
	lib/cksum \
	lib/debugcommands \
	lib/libm \

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

include $(LOCAL_DIR)/uzed.mk

