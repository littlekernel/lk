LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/inc

MODULE_SRCS += \
	$(LOCAL_DIR)/src/stm32f0xx_adc.c \
	$(LOCAL_DIR)/src/stm32f0xx_can.c \
	$(LOCAL_DIR)/src/stm32f0xx_cec.c \
	$(LOCAL_DIR)/src/stm32f0xx_comp.c \
	$(LOCAL_DIR)/src/stm32f0xx_crc.c \
	$(LOCAL_DIR)/src/stm32f0xx_crs.c \
	$(LOCAL_DIR)/src/stm32f0xx_dac.c \
	$(LOCAL_DIR)/src/stm32f0xx_dbgmcu.c \
	$(LOCAL_DIR)/src/stm32f0xx_dma.c \
	$(LOCAL_DIR)/src/stm32f0xx_exti.c \
	$(LOCAL_DIR)/src/stm32f0xx_flash.c \
	$(LOCAL_DIR)/src/stm32f0xx_gpio.c \
	$(LOCAL_DIR)/src/stm32f0xx_i2c.c \
	$(LOCAL_DIR)/src/stm32f0xx_iwdg.c \
	$(LOCAL_DIR)/src/stm32f0xx_misc.c \
	$(LOCAL_DIR)/src/stm32f0xx_pwr.c \
	$(LOCAL_DIR)/src/stm32f0xx_rcc.c \
	$(LOCAL_DIR)/src/stm32f0xx_rtc.c \
	$(LOCAL_DIR)/src/stm32f0xx_spi.c \
	$(LOCAL_DIR)/src/stm32f0xx_syscfg.c \
	$(LOCAL_DIR)/src/stm32f0xx_tim.c \
	$(LOCAL_DIR)/src/stm32f0xx_usart.c \
	$(LOCAL_DIR)/src/stm32f0xx_wwdg.c \
	$(LOCAL_DIR)/src/system_stm32f0xx.c
