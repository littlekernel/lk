# top level project rules for the pc-x86-test project
#
ARCH := x86
SUBARCH := x86-32

include project/target/pc.mk
include project/virtual/test.mk
