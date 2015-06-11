MODULES += \
	lib/debugcommands \
	app/tests \
	app/shell

include project/target/stm32f4-discovery.mk

# Console serial port is on pins PA2(TX) and PA3(RX)
