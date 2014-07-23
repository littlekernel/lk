# top level project rules for the armemu-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \
	app/tests \
	app/stringtests \
	app/shell \
	lib/cksum \
	lib/sysparam \
	lib/debugcommands \
	lib/ptable \

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

include $(LOCAL_DIR)/zybo.mk

