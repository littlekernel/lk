LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := rpi3-vpu

MODULES += \
	app/vc4-stage1 \
	platform/bcm28xx/otp \
	platform/bcm28xx/rpi-ddr2 \

GLOBAL_DEFINES += BOOTCODE=1 WITH_NO_FP=1 NOVM_MAX_ARENAS=2 NOVM_DEFAULT_ARENA=0
BOOTCODE := 1
