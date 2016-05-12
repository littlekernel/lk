/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#ifndef __TARGET_GPIOCONFIG_H
#define __TARGET_GPIOCONFIG_H

#include <platform/gpio.h>
#include <platform/stm32.h>

#define GPIO_LED_ON GPIO_PIN_RESET
#define GPIO_LED_OFF GPIO_PIN_SET

#define GPIO_USART3_TX GPIO(GPIO_PORT_B, 10)
#define GPIO_USART3_RX GPIO(GPIO_PORT_B, 11)

#define GPIO_DISP0_BUSY GPIO(GPIO_PORT_E,4)
#define GPIO_DISP0_CS   GPIO(GPIO_PORT_E,3)
#define GPIO_DISP1_BUSY GPIO(GPIO_PORT_C,11)
#define GPIO_DISP1_CS   GPIO(GPIO_PORT_C,10)

#define GPIO_DISP_RST   GPIO(GPIO_PORT_E,0)
#define GPIO_DISP_DC    GPIO(GPIO_PORT_E,1) 





#define GPIO_SW1 GPIO(GPIO_PORT_J, 0)
#define GPIO_SW2 GPIO(GPIO_PORT_J, 1)
#define GPIO_SW3 GPIO(GPIO_PORT_J, 2)

/**
 *  Definitions for sensor bus signals. Includes
 *   nrf51, accelerometer, and gyroscope
 */

#define __HAL_SENSOR_BUS_GPIO_CLK_ENABLE() \
            __HAL_RCC_GPIOK_CLK_ENABLE(); \
            __HAL_RCC_GPIOF_CLK_ENABLE()

#define GPIO_NRF_CS         GPIO(GPIO_PORT_K, 0)
#define GPIO_NRF_INT        GPIO(GPIO_PORT_K, 1)
#define GPIO_GYRO_nCS       GPIO(GPIO_PORT_K, 4)
#define GPIO_GYRO_INT       GPIO(GPIO_PORT_K, 5)
#define GPIO_ACC_nCS        GPIO(GPIO_PORT_K, 2)
#define GPIO_ACC_INT        GPIO(GPIO_PORT_K, 3)
#define GPIO_SPI5_SCK       GPIO(GPIO_PORT_F, 7)
#define GPIO_SPI5_MISO      GPIO(GPIO_PORT_F, 8)
#define GPIO_SPI5_MOSI      GPIO(GPIO_PORT_F, 9)



#endif
