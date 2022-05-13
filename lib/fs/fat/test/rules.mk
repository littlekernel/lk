LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/fs/fat
MODULE_DEPS += lib/unittest

MODULE_SRCS += $(LOCAL_DIR)/test.cpp

# pass in the local dir relative to the build root
# so the test.cpp has a path to include files against
MODULE_DEFINES += LOCAL_DIR=\"$(LOCAL_DIR)\"

# add a few files that should force a rebuild if modified
MODULE_SRCDEPS += $(LOCAL_DIR)/hello.txt
MODULE_SRCDEPS += $(LOCAL_DIR)/LICENSE

include make/module.mk
