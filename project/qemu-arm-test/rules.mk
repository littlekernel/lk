# top level project rules for the qemu-arm-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := qemu-arm
APPS := tests

OBJS += \
	$(LOCAL_DIR)/init.o

