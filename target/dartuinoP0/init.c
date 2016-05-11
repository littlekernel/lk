/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <err.h>
#include <stdlib.h>
#include <debug.h>
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <dev/gpio.h>
#include <dev/usb.h>
#include <dev/accelerometer.h>
#include <platform/stm32.h>
#include <platform/sdram.h>
#include <platform/gpio.h>
#include <platform/eth.h>
#include <platform/qspi.h>
#include <platform/n25q128a.h>
#include <target/bmi055.h>
#include <target/debugconfig.h>
#include <target/gpioconfig.h>
#include <reg.h>

#if ENABLE_LCD
#include <target/memory_lcd.h>
#endif

#if ENABLE_SENSORBUS
#include <target/sensor_bus.h>
#endif

#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif

#if WITH_LIB_FS_SPIFS
#include <lib/fs.h>
#include <lib/fs/spifs.h>
#define SPIFS_TARGET_DEVICE "qspi-flash"
#endif

extern void target_usb_setup(void);

const sdram_config_t target_sdram_config = {
    .bus_width = SDRAM_BUS_WIDTH_16,
    .cas_latency = SDRAM_CAS_LATENCY_2,
    .col_bits_num = SDRAM_COLUMN_BITS_8
};

void target_early_init(void)
{
    GPIO_InitTypeDef gpio_init;

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();

#if DEBUG_UART == 3
    // configure usart 3 pins.
    gpio_config(GPIO_USART3_TX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF7_USART3) | GPIO_PULLUP);
    gpio_config(GPIO_USART3_RX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF7_USART3) | GPIO_PULLUP);
#else
#error need to configure gpio pins for debug uart
#endif

    gpio_init.Mode    = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull    = GPIO_NOPULL;
    gpio_init.Speed   = GPIO_SPEED_LOW;

    gpio_init.Pin  = GPIO_TO_PIN_MASK(GPIO_LED108) | GPIO_TO_PIN_MASK(GPIO_LED109) |
                     GPIO_TO_PIN_MASK(GPIO_LED110) | GPIO_TO_PIN_MASK(GPIO_LED111);
    HAL_GPIO_Init(GPIOE, &gpio_init);

    gpio_init.Pin  = GPIO_TO_PIN_MASK(GPIO_LED112) | GPIO_TO_PIN_MASK(GPIO_LED113);
    HAL_GPIO_Init(GPIOD, &gpio_init);

    gpio_init.Pin  = GPIO_TO_PIN_MASK(GPIO_LED114) | GPIO_TO_PIN_MASK(GPIO_LED115);
    HAL_GPIO_Init(GPIOJ, &gpio_init);

    // Initialize the switches GPIOs for interrupt on raising edge. In order
    // to use stm32_EXTI15_10_IRQ() handler needs to be provided and EXTI15_10_IRQn
    // needs to be enabled.
    gpio_init.Mode   = GPIO_MODE_INPUT;
    gpio_init.Pull   = GPIO_NOPULL;
    gpio_init.Speed  = GPIO_SPEED_FAST;
    gpio_init.Mode   = GPIO_MODE_IT_RISING;

    gpio_init.Pin  =  GPIO_TO_PIN_MASK(GPIO_SW100) | GPIO_TO_PIN_MASK(GPIO_SW101) |
                      GPIO_TO_PIN_MASK(GPIO_SW102) | GPIO_TO_PIN_MASK(GPIO_SW103);
    HAL_GPIO_Init(GPIOJ, &gpio_init);

#if ENABLE_SENSORBUS
    // Initialize Sensor bus (accelerometer / gyroscope / nrf51 spi bus
    sensor_bus_init_early();
#endif

    // now that the uart gpios are configured, enable the debug uart.
    stm32_debug_early_init();

    // default all the debug leds to off
    target_set_debug_led(0, false);
    target_set_debug_led(1, false);
    target_set_debug_led(2, false);
    target_set_debug_led(3, false);
}


void target_init(void)
{
    stm32_debug_init();

    qspi_flash_init(N25Q128A_FLASH_SIZE);

#if ENABLE_LCD
    memory_lcd_init();
#endif

#if WITH_LIB_MINIP
    uint8_t mac_addr[6];
    gen_random_mac_address(mac_addr);
    eth_init(mac_addr, PHY_KSZ8721);

    /* start minip */
    minip_set_macaddr(mac_addr);

    uint32_t ip_addr = IPV4(192, 168, 0, 98);
    uint32_t ip_mask = IPV4(255, 255, 255, 0);
    uint32_t ip_gateway = IPV4_NONE;

    minip_init(stm32_eth_send_minip_pkt, NULL, ip_addr, ip_mask, ip_gateway);
#endif

#if WITH_LIB_FS_SPIFS
    status_t mount_success = fs_mount(DEAULT_SPIFS_MOUNT_POINT,
                                      DEAULT_SPIFS_NAME, SPIFS_TARGET_DEVICE);
    if (mount_success != NO_ERROR) {
        printf("************************************************************\n");
        printf("failed to mount '%s' at path '%s' on '%s'.\n"
               "Make sure that device is formatted.\n",
               DEAULT_SPIFS_NAME, DEAULT_SPIFS_MOUNT_POINT,
               SPIFS_TARGET_DEVICE);
        printf("Run the following command to format the device then reboot\n"
               "the device to mount the filesystem:\n");
        printf("\t] fs format %s %s\n", DEAULT_SPIFS_NAME,
               SPIFS_TARGET_DEVICE);
        printf("************************************************************\n");

    }

#endif

    // start usb
    target_usb_setup();

#if ENABLE_SENSORBUS
    sensor_bus_init();
#endif
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    if (hspi->Instance == SPI2) {
        /*##-1- Enable peripherals and GPIO Clocks #################################*/
        /* Enable GPIO TX/RX clock */
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPIOK_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /* Enable SPI clock */
        __HAL_RCC_SPI2_CLK_ENABLE();

        /*##-2- Configure peripheral GPIO ##########################################*/
        /* SPI SCK GPIO pin configuration  */
        GPIO_InitStruct.Pin       = GPIO_PIN_3;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* SPI MOSI GPIO pin configuration  */
        GPIO_InitStruct.Pin = GPIO_PIN_15;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;

        /* LCD_ON Pin configuration */
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        HAL_GPIO_Init(GPIOK, &GPIO_InitStruct);

        /* LCD_CS Pin configuration */
        GPIO_InitStruct.Pin = GPIO_PIN_12;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /*##-3- Configure the NVIC for SPI #########################################*/
        /* NVIC for SPI */
        HAL_NVIC_EnableIRQ(SPI2_IRQn);
    }
}

/**
  * @brief  Initializes SDRAM GPIO.
  * called back from stm32_sdram_init
  */
void stm_sdram_GPIO_init(void)
{
    GPIO_InitTypeDef gpio_init_structure;

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /* Common GPIO configuration */
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_PULLUP;
    gpio_init_structure.Speed     = GPIO_SPEED_FAST;
    gpio_init_structure.Alternate = GPIO_AF12_FMC;

    /* GPIOC configuration */
    gpio_init_structure.Pin   = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOC, &gpio_init_structure);

    /* GPIOD configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_8| GPIO_PIN_9 | GPIO_PIN_10 |\
                                GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio_init_structure);

    /* GPIOE configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7| GPIO_PIN_8 | GPIO_PIN_9 |\
                                GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                                GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio_init_structure);

    /* GPIOF configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4 |\
                                GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                                GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &gpio_init_structure);

    /* GPIOG configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4| GPIO_PIN_5 | GPIO_PIN_8 |\
                                GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &gpio_init_structure);

    /* GPIOH configuration */
    gpio_init_structure.Pin   = GPIO_PIN_3 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOH, &gpio_init_structure);
}


/**
  * @brief  Initializes the ETH MSP.
  * @param  heth: ETH handle
  * @retval None
  */
/* called back from the HAL_ETH_Init routine */
void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIOs clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* Ethernet pins configuration ************************************************/
    /*
        RMII_REF_CLK ----------------------> PA1
        RMII_MDIO -------------------------> PA2
        RMII_MDC --------------------------> PC1
        RMII_MII_CRS_DV -------------------> PA7
        RMII_MII_RXD0 ---------------------> PC4
        RMII_MII_RXD1 ---------------------> PC5
        RMII_MII_TX_EN --------------------> PG11
        RMII_MII_TXD0 ---------------------> PG13
        RMII_MII_TXD1 ---------------------> PG14
    */

    /* Configure PA1, PA2 and PA7 */
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Alternate = GPIO_AF11_ETH;
    GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure PC1, PC4 and PC5 */
    GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Configure PG2, PG11, PG13 and PG14 */
    GPIO_InitStructure.Pin =  GPIO_PIN_2 | GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);
}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hqspi)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO clocks */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /*##-2- Configure peripheral GPIO ##########################################*/
    /* QSPI CS GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* QSPI CLK GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* QSPI D0 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_11;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* QSPI D1 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_12;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* QSPI D2 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* QSPI D3 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_13;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/**
  * @brief  Initializes the PCD MSP.
  * @param  hpcd: PCD handle
  * @retval None
  */
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    if (hpcd->Instance == USB_OTG_FS) {
        /* Configure USB FS GPIOs */
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* Configure DM DP Pins */
        GPIO_InitStruct.Pin = (GPIO_PIN_11 | GPIO_PIN_12);
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* Enable USB FS Clock */
        __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

        /* Set USBFS Interrupt priority */
        HAL_NVIC_SetPriority(OTG_FS_IRQn, 5, 0);

        /* Enable USBFS Interrupt */
        HAL_NVIC_EnableIRQ(OTG_FS_IRQn);

        if (hpcd->Init.low_power_enable == 1) {
            /* Enable EXTI Line 18 for USB wakeup*/
            __HAL_USB_OTG_FS_WAKEUP_EXTI_CLEAR_FLAG();
            __HAL_USB_OTG_FS_WAKEUP_EXTI_ENABLE_RISING_EDGE();
            __HAL_USB_OTG_FS_WAKEUP_EXTI_ENABLE_IT();

            /* Set EXTI Wakeup Interrupt priority*/
            HAL_NVIC_SetPriority(OTG_FS_WKUP_IRQn, 0, 0);

            /* Enable EXTI Interrupt */
            HAL_NVIC_EnableIRQ(OTG_FS_WKUP_IRQn);
        }
    } else if (hpcd->Instance == USB_OTG_HS) {
        /* Configure USB FS GPIOs */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOH_CLK_ENABLE();

        /* CLK */
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* D0 */
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* D1 D2 D3 D4 D5 D6 D7 */
        GPIO_InitStruct.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_5 |\
                              GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* STP */
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* NXT */
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
        HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

        /* DIR */
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();

        /* Enable USB HS Clocks */
        __HAL_RCC_USB_OTG_HS_CLK_ENABLE();

        /* Set USBHS Interrupt to the lowest priority */
        HAL_NVIC_SetPriority(OTG_HS_IRQn, 5, 0);

        /* Enable USBHS Interrupt */
        HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
    }
}

void target_set_debug_led(unsigned int led, bool on)
{
    uint32_t gpio;

    switch (led) {
        case 0:
            gpio = GPIO_LED112;
            break;
        case 1:
            gpio = GPIO_LED113;
            break;
        case 2:
            gpio = GPIO_LED114;
            break;
        case 3:
            gpio = GPIO_LED115;
            break;
        default:
            return;
    }

    gpio_set(gpio, on ? GPIO_LED_ON : GPIO_LED_OFF);
}

