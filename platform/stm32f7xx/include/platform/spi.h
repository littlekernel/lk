#pragma once
#include <err.h>
#include <platform/stm32.h>
#include <kernel/mutex.h>

struct spi_bus {
    SPI_TypeDef *spi;
    mutex_t lock;
};

#define INVALID_SPI_BUS -1

status_t spi_init(SPI_HandleTypeDef *handle);
status_t spi_write(SPI_HandleTypeDef *handle, uint8_t *data, size_t len, uint32_t cs);
status_t spi_read(SPI_HandleTypeDef *handle, uint8_t *data, size_t len, uint32_t cs);
status_t spi_transaction(SPI_HandleTypeDef *handle, uint8_t *wdata, uint8_t *rdata, size_t len, uint32_t cs);

