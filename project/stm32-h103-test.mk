LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \
	app/tests \
	app/shell \
	lib/debugcommands \

include project/stm32-h103.mk
