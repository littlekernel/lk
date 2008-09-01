# top level project rules for the armemu-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := beagle
APPS := tests console stringtests
DEVS := 

OBJS += \
	$(LOCAL_DIR)/init.o

