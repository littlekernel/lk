LOCAL_DIR := $(GET_LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/inc

MODULE_SRCS += \
	$(LOCAL_DIR)/src/system_stm32f2xx.c \
	$(LOCAL_DIR)/src/stm32f2xx_dma.c \
	$(LOCAL_DIR)/src/stm32f2xx_dbgmcu.c \
	$(LOCAL_DIR)/src/stm32f2xx_flash.c \
	$(LOCAL_DIR)/src/stm32f2xx_spi.c \
	$(LOCAL_DIR)/src/stm32f2xx_dcmi.c \
	$(LOCAL_DIR)/src/stm32f2xx_adc.c \
	$(LOCAL_DIR)/src/stm32f2xx_iwdg.c \
	$(LOCAL_DIR)/src/stm32f2xx_fsmc.c \
	$(LOCAL_DIR)/src/stm32f2xx_crc.c \
	$(LOCAL_DIR)/src/misc.c \
	$(LOCAL_DIR)/src/stm32f2xx_syscfg.c \
	$(LOCAL_DIR)/src/stm32f2xx_sdio.c \
	$(LOCAL_DIR)/src/stm32f2xx_cryp_aes.c \
	$(LOCAL_DIR)/src/stm32f2xx_usart.c \
	$(LOCAL_DIR)/src/stm32f2xx_exti.c \
	$(LOCAL_DIR)/src/stm32f2xx_rcc.c \
	$(LOCAL_DIR)/src/stm32f2xx_hash_md5.c \
	$(LOCAL_DIR)/src/stm32f2xx_rtc.c \
	$(LOCAL_DIR)/src/stm32f2xx_i2c.c \
	$(LOCAL_DIR)/src/stm32f2xx_cryp_des.c \
	$(LOCAL_DIR)/src/stm32f2xx_rng.c \
	$(LOCAL_DIR)/src/stm32f2xx_cryp_tdes.c \
	$(LOCAL_DIR)/src/stm32f2xx_pwr.c \
	$(LOCAL_DIR)/src/stm32f2xx_wwdg.c \
	$(LOCAL_DIR)/src/stm32f2xx_gpio.c \
	$(LOCAL_DIR)/src/stm32f2xx_hash.c \
	$(LOCAL_DIR)/src/stm32f2xx_can.c \
	$(LOCAL_DIR)/src/stm32f2xx_tim.c \
	$(LOCAL_DIR)/src/stm32f2xx_hash_sha1.c \
	$(LOCAL_DIR)/src/stm32f2xx_cryp.c \
	$(LOCAL_DIR)/src/stm32f2xx_dac.c \
