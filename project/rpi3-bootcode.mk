LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := rpi3-vpu
ARCH := vc4

MODULES += \
	app/stringtests \
	app/rpi-vpu-bootload \
	lib/cksum \
	lib/debugcommands \
	#app/shell \
	#app/tests \

GLOBAL_DEFINES += BOOTCODE=1


BOOTCODE := 1
