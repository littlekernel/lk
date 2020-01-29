LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := rpi3-vpu
ARCH := vc4

MODULES += \
	app/shell \
	app/stringtests \
	lib/cksum \
	lib/debugcommands \
	#app/tests \

GLOBAL_DEFINES += BOOTCODE=1


BOOTCODE := 1
