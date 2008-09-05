# top level project rules for the armemu-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := beagle

MODULES += \
	app/tests \
	app/console \
	app/stringtests

OBJS += \
	$(LOCAL_DIR)/init.o

