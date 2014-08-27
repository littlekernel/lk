# top level project rules for the uzed-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \
	app/stringtests \
	app/shell \
	app/tests \
	app/zynq-common \
	app/lkboot \
	lib/debugcommands

include $(LOCAL_DIR)/uzed.mk

