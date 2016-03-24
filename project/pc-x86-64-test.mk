# top level project rules for the pc-x86-64-test project
#
ARCH := x86
SUBARCH := x86-64
TARGET := pc-x86
MODULES += \
	app/shell

include project/virtual/test.mk
