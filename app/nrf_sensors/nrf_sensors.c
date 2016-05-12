#include <app.h>
#include <arch.h>
#include <assert.h>
#include <debug.h>
#include <err.h>
#include <kernel/event.h>
#include <lk/init.h>
#include <stdlib.h>
#include <trace.h>
#include <target/gpioconfig.h>
#include <stm32f7xx.h>
#include <stm32f7xx_hal_spi.h>
#include <dev/accelerometer.h>


/* This code reads from the accelerometer then sends the data over to the nRF chip
 * every 100 milliseconds. */
static void nrf_sensors_init(const struct app_descriptor *app)
{
}

static void nrf_sensors_entry(const struct app_descriptor *app, void *args)
{
    position_vector_t pos_vector;

    SPI_HandleTypeDef spi_handle;
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

    gpio_config(GPIO_NRF_CS, GPIO_OUTPUT);
    gpio_set(GPIO_NRF_CS, 1);

    for (;;) {
        acc_read_xyz(&pos_vector);
        gpio_set(GPIO_NRF_CS, 0);
        status_t ret = HAL_SPI_Transmit(&spi_handle, (uint8_t *)&pos_vector, sizeof(pos_vector), 5000);
        if (ret != NO_ERROR) {
            printf("error on spi_write for sensors: %d\n", ret);
        }
        gpio_set(GPIO_NRF_CS, 1);
        thread_sleep(100);
    }
}

APP_START(sbb)
    .init  = nrf_sensors_init,
    .entry = nrf_sensors_entry,
APP_END
