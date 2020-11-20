LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := rpi4-vpu

MODULES += \
	app/shell \
	lib/cksum \
	platform/bcm28xx/otp \
	lib/debugcommands \
	app/stringtests \
	app/tests \
	platform/bcm28xx/arm_control \
	platform/bcm28xx/dpi \

