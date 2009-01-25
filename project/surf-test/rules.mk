# top level project rules for the armemu-test project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

ifeq ($(PROJECT_TARGET),)
TARGET := surf-msm7k
else
TARGET := $(PROJECT_TARGET)
endif

MODULES += \
	app/tests \
	lib/console

OBJS += \
	$(LOCAL_DIR)/init.o

