# top level project rules for the armemu-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := dreamcast
APPS := tests \
	console

OBJS += \
	$(LOCAL_DIR)/init.o


