LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/Inc

MODULE_SRCS += \
    $(LOCAL_DIR)/lk_hal.c \
\
    $(LOCAL_DIR)/Src/stm32f7xx_hal.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_adc.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_adc_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_can.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_cec.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_cortex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_crc.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_crc_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_cryp.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_cryp_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_dac.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_dac_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_dcmi.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_dcmi_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_dma.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_dma2d.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_dma_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_eth.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_flash.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_flash_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_gpio.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_hash.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_hash_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_hcd.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_i2c.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_i2c_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_i2s.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_irda.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_iwdg.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_lptim.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_ltdc.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_msp_template.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_nand.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_nor.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_pcd.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_pcd_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_pwr.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_pwr_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_qspi.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_rcc.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_rcc_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_rng.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_rtc.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_rtc_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_sai.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_sai_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_sd.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_sdram.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_smartcard.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_smartcard_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_spdifrx.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_spi.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_sram.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_tim.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_tim_ex.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_uart.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_usart.c \
    $(LOCAL_DIR)/Src/stm32f7xx_hal_wwdg.c \
    $(LOCAL_DIR)/Src/stm32f7xx_ll_fmc.c \
    $(LOCAL_DIR)/Src/stm32f7xx_ll_sdmmc.c \
    $(LOCAL_DIR)/Src/stm32f7xx_ll_usb.c

include $(LOCAL_DIR)/CMSIS/rules.mk

include make/module.mk

