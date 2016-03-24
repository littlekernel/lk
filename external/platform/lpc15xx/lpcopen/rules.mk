LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/lpc_chip_15xx/inc

MODULE_DEFINES += CORE_M3=1

MODULE_SRCS += \
	$(LOCAL_DIR)/lpc_chip_15xx/src/acmp_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/adc_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/chip_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/clock_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/crc_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/dac_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/dma_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/eeprom.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/gpio_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/i2c_common_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/i2cm_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/i2cs_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/iap.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/iocon_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/pinint_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/pmu_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/ring_buffer.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/ritimer_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/rtc_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/sctipu_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/spi_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/stopwatch_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/swm_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/sysctl_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/sysinit_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/uart_15xx.c \
	$(LOCAL_DIR)/lpc_chip_15xx/src/wwdt_15xx.c \

include make/module.mk

