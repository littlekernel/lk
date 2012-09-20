LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \
	lib/debugcommands \
	app/tests \
	app/shell

include project/stm32-p107.mk

