LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/Inc

MODULE_SRCS += \
	$(LOCAL_DIR)/Src/stm32f7xx_hal.c \
	$(LOCAL_DIR)/Src/stm32f7xx_hal_cortex.c \
	$(LOCAL_DIR)/Src/stm32f7xx_hal_gpio.c \
	$(LOCAL_DIR)/Src/stm32f7xx_hal_pwr_ex.c \
	$(LOCAL_DIR)/Src/stm32f7xx_hal_rcc.c \
        $(LOCAL_DIR)/Src/stm32f7xx_hal_rcc_ex.c \
	$(LOCAL_DIR)/Src/stm32f7xx_hal_uart.c \

#MODULE_SRCS += \
	$(LOCAL_DIR)/src/misc.c \
	$(LOCAL_DIR)/src/stm32f4xx_adc.c \
	$(LOCAL_DIR)/src/stm32f4xx_can.c \
	$(LOCAL_DIR)/src/stm32f4xx_cec.c \
	$(LOCAL_DIR)/src/stm32f4xx_crc.c \
	$(LOCAL_DIR)/src/stm32f4xx_cryp_aes.c \
	$(LOCAL_DIR)/src/stm32f4xx_cryp.c \
	$(LOCAL_DIR)/src/stm32f4xx_cryp_des.c \
	$(LOCAL_DIR)/src/stm32f4xx_cryp_tdes.c \
	$(LOCAL_DIR)/src/stm32f4xx_dac.c \
	$(LOCAL_DIR)/src/stm32f4xx_dbgmcu.c \
	$(LOCAL_DIR)/src/stm32f4xx_dcmi.c \
	$(LOCAL_DIR)/src/stm32f4xx_dma2d.c \
	$(LOCAL_DIR)/src/stm32f4xx_dma.c \
	$(LOCAL_DIR)/src/stm32f4xx_exti.c \
	$(LOCAL_DIR)/src/stm32f4xx_flash.c \
	$(LOCAL_DIR)/src/stm32f4xx_flash_ramfunc.c \
	$(LOCAL_DIR)/src/stm32f4xx_fmpi2c.c \
	$(LOCAL_DIR)/src/stm32f4xx_fsmc.c \
	$(LOCAL_DIR)/src/stm32f4xx_gpio.c \
	$(LOCAL_DIR)/src/stm32f4xx_hash.c \
	$(LOCAL_DIR)/src/stm32f4xx_hash_md5.c \
	$(LOCAL_DIR)/src/stm32f4xx_hash_sha1.c \
	$(LOCAL_DIR)/src/stm32f4xx_i2c.c \
	$(LOCAL_DIR)/src/stm32f4xx_iwdg.c \
	$(LOCAL_DIR)/src/stm32f4xx_ltdc.c \
	$(LOCAL_DIR)/src/stm32f4xx_pwr.c \
	$(LOCAL_DIR)/src/stm32f4xx_qspi.c \
	$(LOCAL_DIR)/src/stm32f4xx_rcc.c \
	$(LOCAL_DIR)/src/stm32f4xx_rng.c \
	$(LOCAL_DIR)/src/stm32f4xx_rtc.c \
	$(LOCAL_DIR)/src/stm32f4xx_sai.c \
	$(LOCAL_DIR)/src/stm32f4xx_sdio.c \
	$(LOCAL_DIR)/src/stm32f4xx_spdifrx.c \
	$(LOCAL_DIR)/src/stm32f4xx_spi.c \
	$(LOCAL_DIR)/src/stm32f4xx_syscfg.c \
	$(LOCAL_DIR)/src/stm32f4xx_tim.c \
	$(LOCAL_DIR)/src/stm32f4xx_usart.c \
	$(LOCAL_DIR)/src/stm32f4xx_wwdg.c \
	$(LOCAL_DIR)/src/system_stm32f4xx.c
