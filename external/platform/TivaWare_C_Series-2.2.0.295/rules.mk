LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)

# TI's driverlib wants this
MODULE_COMPILEFLAGS += -Dgcc

MODULE_SRCS += \
	$(LOCAL_DIR)/driverlib/adc.c \
	$(LOCAL_DIR)/driverlib/aes.c \
	$(LOCAL_DIR)/driverlib/can.c \
	$(LOCAL_DIR)/driverlib/comp.c \
	$(LOCAL_DIR)/driverlib/cpu.c \
	$(LOCAL_DIR)/driverlib/crc.c \
	$(LOCAL_DIR)/driverlib/des.c \
	$(LOCAL_DIR)/driverlib/eeprom.c \
	$(LOCAL_DIR)/driverlib/emac.c \
	$(LOCAL_DIR)/driverlib/epi.c \
	$(LOCAL_DIR)/driverlib/flash.c \
	$(LOCAL_DIR)/driverlib/fpu.c \
	$(LOCAL_DIR)/driverlib/gpio.c \
	$(LOCAL_DIR)/driverlib/hibernate.c \
	$(LOCAL_DIR)/driverlib/i2c.c \
	$(LOCAL_DIR)/driverlib/interrupt.c \
	$(LOCAL_DIR)/driverlib/lcd.c \
	$(LOCAL_DIR)/driverlib/mpu.c \
	$(LOCAL_DIR)/driverlib/onewire.c \
	$(LOCAL_DIR)/driverlib/pwm.c \
	$(LOCAL_DIR)/driverlib/qei.c \
	$(LOCAL_DIR)/driverlib/shamd5.c \
	$(LOCAL_DIR)/driverlib/ssi.c \
	$(LOCAL_DIR)/driverlib/sw_crc.c \
	$(LOCAL_DIR)/driverlib/sysctl.c \
	$(LOCAL_DIR)/driverlib/sysexc.c \
	$(LOCAL_DIR)/driverlib/timer.c \
	$(LOCAL_DIR)/driverlib/uart.c \
	$(LOCAL_DIR)/driverlib/udma.c \
	$(LOCAL_DIR)/driverlib/usb.c \
	$(LOCAL_DIR)/driverlib/watchdog.c

# UNUSED
#	$(LOCAL_DIR)/driverlib/systick.c

include make/module.mk


