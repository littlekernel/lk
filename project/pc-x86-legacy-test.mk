# top level project rules for the pc-x86-legacy-test project
#
ARCH := x86
CPU ?= legacy

include project/target/pc.mk
include project/virtual/fs.mk
include project/virtual/test.mk
