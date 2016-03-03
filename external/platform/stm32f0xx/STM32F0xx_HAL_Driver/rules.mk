LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/inc

MODULE_SRCS += $(LOCAL_DIR)/stm32f0xx_hal.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_adc.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_adc_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_can.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_cec.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_comp.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_cortex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_crc.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_crc_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_dac.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_dac_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_dma.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_flash.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_flash_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_gpio.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_i2c.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_i2c_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_i2s.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_irda.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_iwdg.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_msp_template.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_pcd.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_pcd_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_pwr.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_pwr_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_rcc.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_rcc_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_rtc.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_rtc_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_smartcard.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_smartcard_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_smbus.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_spi.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_spi_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_tim.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_tim_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_tsc.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_uart.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_uart_ex.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_usart.c \
							 $(LOCAL_DIR)/stm32f0xx_hal_wwdg.c

include make/module.mk
