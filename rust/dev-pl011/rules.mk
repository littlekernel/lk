LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

RUST_CRATES += $(LOCAL_DIR)

# add any dependencies needed for rust support
MODULE_DEPS += lib/rust_support

# make sure the local include directory is in the global include path
include make/module.mk
