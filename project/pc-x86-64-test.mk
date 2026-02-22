# top level project rules for the pc-x86-64-test project
#
ARCH := x86
SUBARCH := x86-64

include project/target/pc.mk
include project/virtual/fs.mk
include project/virtual/minip.mk
include project/virtual/test.mk

USE_RUST ?= 0

ifeq ($(call TOBOOL,$(USE_RUST)),true)
$(info "Including rust support")
include project/virtual/rust.mk
endif
