include project/target/nextcoinP0.mk
include project/virtual/test.mk

# Console serial port is on pins PA9(TX) and PB7(RX)

include project/virtual/fs.mk


MODULES += \
	target/nextcoin/projects/system \
	app/nrf_sensors

MODULE_DEPS += \
    app/accelerometer \

MODULE_SRCS += \
    $(LOCAL_DIR)/sensor_bus.c \

GLOBAL_DEFINES += \
	ENABLE_SENSORBUS=1
