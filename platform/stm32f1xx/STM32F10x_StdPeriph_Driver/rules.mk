LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/inc

MODULE_SRCS += \
	$(LOCAL_DIR)/src/misc.c \
	$(LOCAL_DIR)/src/stm32f10x_adc.c \
	$(LOCAL_DIR)/src/stm32f10x_bkp.c \
	$(LOCAL_DIR)/src/stm32f10x_can.c \
	$(LOCAL_DIR)/src/stm32f10x_cec.c \
	$(LOCAL_DIR)/src/stm32f10x_crc.c \
	$(LOCAL_DIR)/src/stm32f10x_dac.c \
	$(LOCAL_DIR)/src/stm32f10x_dbgmcu.c \
	$(LOCAL_DIR)/src/stm32f10x_dma.c \
	$(LOCAL_DIR)/src/stm32f10x_exti.c \
	$(LOCAL_DIR)/src/stm32f10x_flash.c \
	$(LOCAL_DIR)/src/stm32f10x_fsmc.c \
	$(LOCAL_DIR)/src/stm32f10x_gpio.c \
	$(LOCAL_DIR)/src/stm32f10x_i2c.c \
	$(LOCAL_DIR)/src/stm32f10x_iwdg.c \
	$(LOCAL_DIR)/src/stm32f10x_pwr.c \
	$(LOCAL_DIR)/src/stm32f10x_rcc.c \
	$(LOCAL_DIR)/src/stm32f10x_rtc.c \
	$(LOCAL_DIR)/src/stm32f10x_sdio.c \
	$(LOCAL_DIR)/src/stm32f10x_spi.c \
	$(LOCAL_DIR)/src/stm32f10x_tim.c \
	$(LOCAL_DIR)/src/stm32f10x_usart.c \
	$(LOCAL_DIR)/src/stm32f10x_wwdg.c \
	$(LOCAL_DIR)/src/system_stm32f10x.c
