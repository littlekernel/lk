# $(error lib/rust_support/rules.mk invoked)

LOCAL_DIR := $(GET_LOCAL_DIR)
CRATE_NAME := rust_support

MODULE := $(LOCAL_DIR)

# Rust specific module support.
# Although this starts here, this should be generalized, possibly into a module_rust.mk.

MODULE_SRCDIR := $(MODULE)
MODULE_BUILDDIR := $(call TOBUILDDIR,$(MODULE_SRCDIR))

# TODO These should be set by the arch.
RUST_TARGET := aarch64-unknown-none

# This name is actually from the Cargo.toml file, for now assume it is the same.
MODULE_OBJECT := $(call TOBUILDDIR,$(MODULE_SRCDIR)/$(RUST_TARGET)/debug/lib$(CRATE_NAME).a)

$(MODULE_OBJECT).phony:
.PHONY: $(MODULE_OBJECT).phony

# Cargo will use paths relative to the working dir, so this needs to be absolute
$(MODULE_OBJECT): MODULE_BUILDDIR:=$(abspath $(MODULE_BUILDDIR))

$(MODULE_OBJECT): MODULE_SRCDIR:=$(MODULE_SRCDIR)

$(MODULE_OBJECT): $(MODULE_OBJECT).phony
	cd $(MODULE_SRCDIR); \
		env RUSTFLAGS="-C link-self-contained=no" \
		GLOBAL_INCLUDES="$(GLOBAL_INCLUDES)" \
		TARGET="$(RUST_TARGET)" \
		cargo build \
		--target $(RUST_TARGET) \
		--target-dir $(MODULE_BUILDDIR)

# ALLMODULE_OBJS := $(ALLMODULE_OBJS) $(MODULE_OBJECT)
EXTRA_OBJS := $(EXTRA_OBJS) $(MODULE_OBJECT)

MODULE :=
MODULE_SRCDIR :=
MODULE_BUILDDIR :=
MODULE_OBJECT :=
