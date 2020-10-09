LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := rpi2

MODULES += \
	app/shell \
	app/stringtests \
	app/tests \
	lib/cksum \
	lib/debugcommands \
	platform/bcm28xx/pll \
	platform/bcm28xx/hvs-dance \
	lib/tga \
