LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/driverlib/adc.c \
	$(LOCAL_DIR)/driverlib/can.c \
	$(LOCAL_DIR)/driverlib/comp.c \
	$(LOCAL_DIR)/driverlib/cpu.c \
	$(LOCAL_DIR)/driverlib/eeprom.c \
	$(LOCAL_DIR)/driverlib/epi.c \
	$(LOCAL_DIR)/driverlib/ethernet.c \
	$(LOCAL_DIR)/driverlib/fan.c \
	$(LOCAL_DIR)/driverlib/flash.c \
	$(LOCAL_DIR)/driverlib/fpu.c \
	$(LOCAL_DIR)/driverlib/gpio.c \
	$(LOCAL_DIR)/driverlib/hibernate.c \
	$(LOCAL_DIR)/driverlib/i2c.c \
	$(LOCAL_DIR)/driverlib/i2s.c \
	$(LOCAL_DIR)/driverlib/interrupt.c \
	$(LOCAL_DIR)/driverlib/lpc.c \
	$(LOCAL_DIR)/driverlib/mpu.c \
	$(LOCAL_DIR)/driverlib/peci.c \
	$(LOCAL_DIR)/driverlib/pwm.c \
	$(LOCAL_DIR)/driverlib/qei.c \
	$(LOCAL_DIR)/driverlib/ssi.c \
	$(LOCAL_DIR)/driverlib/sysctl.c \
	$(LOCAL_DIR)/driverlib/sysexc.c \
	$(LOCAL_DIR)/driverlib/timer.c \
	$(LOCAL_DIR)/driverlib/uart.c \
	$(LOCAL_DIR)/driverlib/udma.c \
	$(LOCAL_DIR)/driverlib/usb.c \
	$(LOCAL_DIR)/driverlib/watchdog.c

# UNUSED
#	$(LOCAL_DIR)/driverlib/systick.c
