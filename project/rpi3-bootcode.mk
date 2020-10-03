LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := rpi3-vpu
ARCH := vc4

MODULES += \
	app/shell \
	platform/bcm28xx/rpi-ddr2/autoram \
	#platform/bcm28xx/dpi \
	#platform/bcm28xx/otp \
	#lib/debugcommands \
	#app/shell \
	#app/tests \
	#app/stringtests \
	#app/rpi-vpu-bootload \
	#lib/cksum \

GLOBAL_DEFINES += BOOTCODE=1


BOOTCODE := 1
