# top level project rules for the pc virtual project
#
ARCH ?= x86
SUBARCH ?= x86-32
TARGET := pc-x86

CPU ?= legacy

MODULES += \
	app/shell
