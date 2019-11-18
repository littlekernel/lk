# main project for TI Hercules TMS570LS12x LaunchPad
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := tms570

MODULES += \
	app/shell \
	app/stringtests \
	app/tests \
	lib/cksum \
	lib/debugcommands \

