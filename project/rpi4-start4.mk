LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := rpi4-vpu
ARCH := vc4

MODULES += \
	app/shell \
	lib/cksum \
	platform/bcm28xx/otp \
	lib/debugcommands \
	app/stringtests \
	app/tests \
	platform/bcm28xx/arm_control \

