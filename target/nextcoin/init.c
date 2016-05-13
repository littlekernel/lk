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
#include <platform/gpio.h>
#include <platform/qspi.h>
#include <platform/n25q128a.h>
#include <target/bmi055.h>
#include <target/debugconfig.h>
#include <target/gpioconfig.h>
#include <target/et011tt2v1.h>
#include <reg.h>

#if ENABLE_SENSORBUS
#include <target/sensor_bus.h>
#endif

#if WITH_LIB_FS_SPIFS
#include <lib/fs.h>
#include <lib/fs/spifs.h>
#define SPIFS_TARGET_DEVICE "qspi-flash"
#endif

extern void target_usb_setup(void);

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

    // Initialize the switches GPIOs for interrupt on raising edge. In order
    // to use stm32_EXTI15_10_IRQ() handler needs to be provided and EXTI15_10_IRQn
    // needs to be enabled.
    gpio_init.Mode   = GPIO_MODE_INPUT;
    gpio_init.Pull   = GPIO_NOPULL;
    gpio_init.Speed  = GPIO_SPEED_FAST;
    gpio_init.Mode   = GPIO_MODE_IT_RISING;

    gpio_init.Pin  =  GPIO_TO_PIN_MASK(GPIO_SW1) | GPIO_TO_PIN_MASK(GPIO_SW2) |
                      GPIO_TO_PIN_MASK(GPIO_SW2) ;
    HAL_GPIO_Init(GPIOJ, &gpio_init);

#if ENABLE_SENSORBUS
    // Initialize Sensor bus (accelerometer / gyroscope / nrf51 spi bus
    sensor_bus_init_early();
#endif

    // now that the uart gpios are configured, enable the debug uart.
    stm32_debug_early_init();

    // default all the debug leds to off
}


void target_init(void)
{
    stm32_debug_init();

    qspi_flash_init(N25Q128A_FLASH_SIZE);

#if WITH_LIB_FS_SPIFS
    status_t mount_success = fs_mount(DEAULT_SPIFS_MOUNT_POINT,
                                      DEAULT_SPIFS_NAME, SPIFS_TARGET_DEVICE);
    if (mount_success != NO_ERROR) {
        printf("failed to mount '%s' at path '%s' on '%s'."
               " Make sure that device is formatted\n",
               DEAULT_SPIFS_NAME, DEAULT_SPIFS_MOUNT_POINT,
               SPIFS_TARGET_DEVICE);
    }

#endif

    // start usb
    target_usb_setup();

#if ENABLE_SENSORBUS
    sensor_bus_init();
#endif
#if WITH_LIB_CONSOLE    
    eink_init();
#endif
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    if (hspi->Instance == SPI2) {
        /* SPI2
         * PB15 MOSI
         * PD3  SCK
         * PE3  CS0 (soft)
         * PC10 CS1 (soft)

         */
        printf("Configuring SPI2.\n");
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPIOI_CLK_ENABLE();
        __HAL_RCC_GPIOK_CLK_ENABLE();
        __HAL_RCC_SPI2_CLK_ENABLE();



        
        GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
                /* SPI2 CS1 GPIO pin configuration (general output GPIO) */
        GPIO_InitStruct.Pin       = GPIO_PIN_10;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);

        /* SPI2 CS0 GPIO pin configuration (general output GPIO) */
        GPIO_InitStruct.Pin       = GPIO_PIN_3;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);

        /* DISP_DC pin configuration (general output GPIO) */
        GPIO_InitStruct.Pin       = GPIO_PIN_1;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);

        /* DISP_RESET pin configuration (general output GPIO) */
        GPIO_InitStruct.Pin       = GPIO_PIN_0;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_RESET);


        /* DISP_BUSY0/1 GPIO pin configuration (general output GPIO) */
        //GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Pin       = GPIO_PIN_4;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
        GPIO_InitStruct.Pin       = GPIO_PIN_11;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);


        /* Common SPI2 AF config */
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = 0x2; // GPIO_PULLDOWN
        GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        /* SPI2 MOSI GPIO pin configuration  */
        GPIO_InitStruct.Pin       = GPIO_PIN_15;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        /* SPI2 SCK GPIO pin configuration */
        GPIO_InitStruct.Pin       = GPIO_PIN_3;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);



        /* NVIC for SPI */
        HAL_NVIC_EnableIRQ(SPI2_IRQn);
    } else if (hspi->Instance == SPI5) {
        /* SPI5
         * PF6 SPI5_NSS
         * PF7 SPI5_SCK
         * PF8 SPI5_MISO
         * PF9 SPI5_MOSI
         */
        __HAL_RCC_GPIOF_CLK_ENABLE();
        __HAL_RCC_SPI5_CLK_ENABLE();
        gpio_config(GPIO_SPI5_SCK,  GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF5_SPI5) | GPIO_PULLUP);
        gpio_config(GPIO_SPI5_MISO, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF5_SPI5) | GPIO_PULLUP);
        gpio_config(GPIO_SPI5_MOSI, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF5_SPI5) | GPIO_PULLUP);
        HAL_NVIC_EnableIRQ(SPI5_IRQn);
    }
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
    }
}


