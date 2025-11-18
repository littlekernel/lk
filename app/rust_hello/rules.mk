LOCAL_DIR := $(GET_LOCAL_DIR)

RUST_CRATES += $(LOCAL_DIR)
$(info "Adding rust crate $(LOCAL_DIR)")

MODULE := $(LOCAL_DIR)

MODULE_DEPS := lib/rust_support

include make/module.mk
