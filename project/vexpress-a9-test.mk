# top level project rules for the armemu-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := vexpress-a9

MODULES += \
	app/tests \
	app/stringtests \
	app/shell \
	lib/debugcommands


