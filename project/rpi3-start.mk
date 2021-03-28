LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := rpi3-vpu

MODULES += \
	app/shell \
	app/stringtests \
	app/tests \
	lib/cksum \
	lib/debugcommands \
	platform/bcm28xx/otp \
	platform/bcm28xx/vec \
	platform/bcm28xx/hvs-dance \
