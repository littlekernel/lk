/*
 * Copyright (c) 2015 Eric Holland
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
#include <kernel/mutex.h>
#include <platform/gpio.h>
#include <target/gpioconfig.h>
#include <target/bmi055.h>
#include <target/sensor_bus.h>
#include <dev/accelerometer.h>

static mutex_t sensorbus_mutex;
static SPI_HandleTypeDef spi_handle;

static uint8_t tx_buff[16];
static uint8_t rx_buff[16];

status_t acc_read_xyz(position_vector_t *pos_vector_p)
{
    tx_buff[0] = BMI055_ADDRESS_READ( BMI055_ACC_ACCD_X_LSB );
    if ( acc_flush(tx_buff, rx_buff, 7) == NO_ERROR ) {
        pos_vector_p->x = 0.001*(((int8_t)rx_buff[2] << 4) | ( (rx_buff[1] >> 4) & 0x0F));
        pos_vector_p->y = 0.001*(((int8_t)rx_buff[4] << 4) | ( (rx_buff[3] >> 4) & 0x0F));
        pos_vector_p->z = 0.001*(((int8_t)rx_buff[6] << 4) | ( (rx_buff[5] >> 4) & 0x0F));
        return NO_ERROR;
    } else {
        return ERR_GENERIC;
    }
}

status_t acc_flush(uint8_t *tbuff, uint8_t *rbuff, uint8_t numbytes)
{
    status_t ret_status;

    mutex_acquire(&sensorbus_mutex);

    gpio_set(GPIO_ACC_nCS,GPIO_PIN_RESET);

    ret_status = HAL_SPI_TransmitReceive(&spi_handle, tbuff, rbuff, numbytes, 5000);

    gpio_set(GPIO_ACC_nCS,GPIO_PIN_SET);

    mutex_release(&sensorbus_mutex);

    return ret_status;
}

/**
 * @brief Initiale SPI5 module and IO for control of spi bus linking nrf51, accelerometer, and gyroscope.
 *
 */
status_t sensor_bus_init_early(void)
{
    __HAL_SENSOR_BUS_GPIO_CLK_ENABLE();
    __HAL_RCC_SPI5_CLK_ENABLE();

    gpio_config(GPIO_SPI5_SCK,  GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF5_SPI5) | GPIO_PULLUP);
    gpio_config(GPIO_SPI5_MISO, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF5_SPI5) | GPIO_PULLUP);
    gpio_config(GPIO_SPI5_MOSI, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF5_SPI5) | GPIO_PULLUP);

    gpio_config(GPIO_NRF_CS,    GPIO_OUTPUT );
    gpio_config(GPIO_NRF_INT,   GPIO_INPUT   | GPIO_PULLUP);

    gpio_config(GPIO_GYRO_nCS,  GPIO_OUTPUT );
    gpio_config(GPIO_GYRO_INT,  GPIO_INPUT   | GPIO_PULLUP);

    gpio_config(GPIO_ACC_nCS,   GPIO_OUTPUT );
    gpio_config(GPIO_ACC_INT,   GPIO_INPUT   | GPIO_PULLUP);

    gpio_set(GPIO_NRF_CS, GPIO_PIN_RESET);
    gpio_set(GPIO_GYRO_nCS, GPIO_PIN_SET);
    gpio_set(GPIO_ACC_nCS, GPIO_PIN_SET);

    spi_handle.Instance               = SPI5;
    spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
    spi_handle.Init.Direction         = SPI_DIRECTION_2LINES;
    spi_handle.Init.CLKPhase          = SPI_PHASE_1EDGE;
    spi_handle.Init.CLKPolarity       = SPI_POLARITY_LOW;
    spi_handle.Init.DataSize          = SPI_DATASIZE_8BIT;
    spi_handle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    spi_handle.Init.TIMode            = SPI_TIMODE_DISABLE;
    spi_handle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    spi_handle.Init.CRCPolynomial     = 7;
    spi_handle.Init.NSS               = SPI_NSS_SOFT;
    spi_handle.Init.Mode              = SPI_MODE_MASTER;

    if (HAL_SPI_Init(&spi_handle) != HAL_OK) {
        return ERR_GENERIC;
    }
    return NO_ERROR;
}



void sensor_bus_init(void)
{
    mutex_init(&sensorbus_mutex);
}


