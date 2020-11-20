LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := rpi3-vpu

MODULES += \
	app/vc4-stage2 \
	platform/bcm28xx/otp \
	#lib/debugcommands \
	lib/cksum \
	app/stringtests \
	app/tests \

#ARCH_COMPILEFLAGS += -fPIE
#ARCH_LDFLAGS += --emit-relocs --discard-none
#ARCH_LDFLAGS += --emit-relocs --discard-none --export-dynamic
