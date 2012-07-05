LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/inc

OBJS += \
	$(LOCAL_DIR)/src/misc.o \
	$(LOCAL_DIR)/src/stm32f10x_adc.o \
	$(LOCAL_DIR)/src/stm32f10x_bkp.o \
	$(LOCAL_DIR)/src/stm32f10x_can.o \
	$(LOCAL_DIR)/src/stm32f10x_cec.o \
	$(LOCAL_DIR)/src/stm32f10x_crc.o \
	$(LOCAL_DIR)/src/stm32f10x_dac.o \
	$(LOCAL_DIR)/src/stm32f10x_dbgmcu.o \
	$(LOCAL_DIR)/src/stm32f10x_dma.o \
	$(LOCAL_DIR)/src/stm32f10x_exti.o \
	$(LOCAL_DIR)/src/stm32f10x_flash.o \
	$(LOCAL_DIR)/src/stm32f10x_fsmc.o \
	$(LOCAL_DIR)/src/stm32f10x_gpio.o \
	$(LOCAL_DIR)/src/stm32f10x_i2c.o \
	$(LOCAL_DIR)/src/stm32f10x_iwdg.o \
	$(LOCAL_DIR)/src/stm32f10x_pwr.o \
	$(LOCAL_DIR)/src/stm32f10x_rcc.o \
	$(LOCAL_DIR)/src/stm32f10x_rtc.o \
	$(LOCAL_DIR)/src/stm32f10x_sdio.o \
	$(LOCAL_DIR)/src/stm32f10x_spi.o \
	$(LOCAL_DIR)/src/stm32f10x_tim.o \
	$(LOCAL_DIR)/src/stm32f10x_usart.o \
	$(LOCAL_DIR)/src/stm32f10x_wwdg.o \
	$(LOCAL_DIR)/src/system_stm32f10x.o
