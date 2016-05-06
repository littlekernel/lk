#include <assert.h>
#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <target/gpioconfig.h>
#include <platform/spi.h>
#include <platform/stm32.h>

static struct spi_bus spi_busses[] = {
    { .spi = SPI1 },
    { .spi = SPI2 },
    { .spi = SPI3 },
    { .spi = SPI4 },
    { .spi = SPI5 },
    { .spi = SPI6 }
};

static inline int get_bus_idx(SPI_TypeDef *spi)
{
    for (unsigned int i = 0; i < (sizeof(spi_busses)/sizeof(spi_busses[0])); i++) {
        if (spi_busses[i].spi == spi) {
            return i;
        }
    }

    return INVALID_SPI_BUS;
}

static bool wait_for_bus_ready(SPI_HandleTypeDef *handle)
{
    size_t timeout = 1024;
    while (timeout && (HAL_SPI_GetState(handle) != HAL_SPI_STATE_READY)) {
        timeout--;
    }

    if (timeout == 0) {
        return false;
    }

    return true;
}

status_t spi_init (SPI_HandleTypeDef *handle)
{
    DEBUG_ASSERT(handle);

    status_t ret = NO_ERROR;
    int bus = get_bus_idx(handle->Instance);

    if (bus == INVALID_SPI_BUS) {
        return ERR_NOT_FOUND;
    }

    mutex_init(&(spi_busses[bus].lock));
    if (HAL_SPI_Init(handle) != HAL_OK) {
        ret = ERR_GENERIC;
    }

    return ret;
}

status_t spi_write(SPI_HandleTypeDef *handle, uint8_t *data, size_t len, uint32_t cs)
{
    DEBUG_ASSERT(handle);
    DEBUG_ASSERT(data);
    DEBUG_ASSERT(len);

    status_t ret = NO_ERROR;
    int bus = get_bus_idx(handle->Instance);
    int cs_val;
    bool use_soft_nss = ((handle->Init.NSS == SPI_NSS_SOFT) && cs);

    if (bus == INVALID_SPI_BUS) {
        return ERR_NOT_FOUND;
    }

    mutex_acquire(&(spi_busses[bus].lock));
    if (use_soft_nss) {
        cs_val = gpio_get(cs);
        gpio_set(cs, !cs_val);
    }

    HAL_StatusTypeDef foo;

    /* The lock may have been released while the hardware is still processing a transaction */
    if (!wait_for_bus_ready(handle)) {
        ret = ERR_BUSY;
    } else {
        if ((foo = HAL_SPI_Transmit(handle, data, len, HAL_MAX_DELAY)) != HAL_OK) {
            ret = ERR_IO;
        }
    }

    if (use_soft_nss) {
        gpio_set(cs, cs_val);
    }
    mutex_release(&(spi_busses[bus].lock));

    return ret;
}

status_t spi_read(SPI_HandleTypeDef *handle, uint8_t *data, size_t len, uint32_t cs)
{
    DEBUG_ASSERT(handle);
    DEBUG_ASSERT(data);
    DEBUG_ASSERT(len);

    status_t ret = NO_ERROR;
    int bus = get_bus_idx(handle->Instance);
    int cs_val;
    bool use_soft_nss = (handle->Init.NSS == SPI_NSS_SOFT);

    if (bus == INVALID_SPI_BUS) {
        return ERR_NOT_FOUND;
    }

    mutex_acquire(&(spi_busses[bus].lock));
    if (use_soft_nss) {
        cs_val = gpio_get(cs);
        gpio_set(cs, !cs_val);
    }

    if (HAL_SPI_Receive(handle, data, len, HAL_MAX_DELAY) != HAL_OK) {
        ret = ERR_IO;
    }

    if (use_soft_nss) {
        gpio_set(cs, cs_val);
    }
    mutex_release(&(spi_busses[bus].lock));

    return ret;
}

status_t spi_transaction(SPI_HandleTypeDef *handle, uint8_t *wdata, uint8_t *rdata, size_t len, uint32_t cs)
{
    DEBUG_ASSERT(handle);
    DEBUG_ASSERT(wdata);
    DEBUG_ASSERT(rdata);
    DEBUG_ASSERT(len);

    status_t ret = NO_ERROR;
    int bus = get_bus_idx(handle->Instance);

    if (bus == INVALID_SPI_BUS) {
        return ERR_NOT_FOUND;
    }

    mutex_acquire(&(spi_busses[bus].lock));
    if (handle->Init.NSS == SPI_NSS_SOFT) {
        gpio_set(cs, (handle->Init.CLKPolarity == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }

    if (HAL_SPI_TransmitReceive(handle, wdata, rdata, len, HAL_MAX_DELAY) != HAL_OK) {
        ret = ERR_IO;
    }

    if (handle->Init.NSS == SPI_NSS_SOFT) {
        gpio_set(cs, (handle->Init.CLKPolarity == 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    mutex_release(&(spi_busses[bus].lock));

    return ret;
}

#if WITH_LIB_CONSOLE
#include <lib/console.h>

/* Bus configurations for testing. Although it requires a recompilation per setting tweaked,
 * it saves us from having to write a great deal of configuration code via an awkward console
 * interface with many arguments.
 *
 * XXX: This doesn't handle soft NSS on its own.
 */

// This test code is primarily for DartuinoP0 and the GPIO will only exist for that target
#define STM32F7_SPI_BUS_CNT     6
static SPI_HandleTypeDef handles[STM32F7_SPI_BUS_CNT] = {
    [0] = { 0 },
    [1] = {
        .Instance               = SPI2,
        .Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4,
        .Init.Direction         = SPI_DIRECTION_1LINE,
        .Init.CLKPhase          = SPI_PHASE_1EDGE,
        .Init.CLKPolarity       = SPI_POLARITY_LOW,
        .Init.DataSize          = SPI_DATASIZE_8BIT,
        .Init.FirstBit          = SPI_FIRSTBIT_LSB,
        .Init.TIMode            = SPI_TIMODE_DISABLE,
        .Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE,
        .Init.CRCPolynomial     = 7,
        .Init.NSS               = SPI_NSS_SOFT,
        .Init.Mode              = SPI_MODE_MASTER,
    },
    [2] = { 0 },
    [3] = { 0 },
    [4] = { 0 },
    [5] = {
        .Instance               = SPI5,
        .Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4,
        .Init.Direction         = SPI_DIRECTION_1LINE,
        .Init.CLKPhase          = SPI_PHASE_1EDGE,
        .Init.CLKPolarity       = SPI_POLARITY_LOW,
        .Init.DataSize          = SPI_DATASIZE_8BIT,
        .Init.FirstBit          = SPI_FIRSTBIT_MSB,
        .Init.TIMode            = SPI_TIMODE_DISABLE,
        .Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE,
        .Init.CRCPolynomial     = 7,
        .Init.NSS               = SPI_NSS_SOFT,
        .Init.Mode              = SPI_MODE_MASTER,
    },
};

// GPIO to whack for soft chip select
static unsigned int handle_nss[STM32F7_SPI_BUS_CNT] = {
    [5] = GPIO(GPIO_PORT_K, 0), // NRF_CS on Dartuino P0
};


/* Check if bus is valid, initialize it if needed */
static inline bool check_bus(uint8_t bus)
{
    if (bus < STM32F7_SPI_BUS_CNT && handles[bus].Instance == 0) {
        return false;
    }

    if (HAL_SPI_GetState(&handles[bus]) == HAL_SPI_STATE_RESET) {
        HAL_SPI_Init(&handles[bus]);
    }

    return true;
}

static void print_spi_usage(void)
{
    printf("spi read <bus> <len>\n");
    printf("\tRead <len> bytes from <bus>\n");
    printf("spi spew <bus> <len> <n> <delay>\n");
    printf("\tWrite <len> bytes <n> times with a delay of <delay> ms between over <bus>\n");
}

#define IS_CMD(_str, _argc) (argc >= _argc && (strcmp(_str, argv[1].str) == 0))
static int cmd_spi(int argc, const cmd_args *argv)
{
    status_t ret = NO_ERROR;
    // spi read <bus> <len>
    if (IS_CMD("read", 4)) {
        uint8_t bus = argv[2].i;
        size_t len = MIN(1024, argv[3].i);
        uint8_t *data = (uint8_t *)malloc(len);

        if (!data) {
            ret = ERR_NO_MEMORY;
            goto read_err;
        }

        if (!check_bus(bus)) {
            printf("Error: bus %u is invalid\n", bus);
            ret = ERR_INVALID_ARGS;
            goto read_err;
        }

        if (spi_read(&handles[bus], data, len, handle_nss[bus]) != NO_ERROR) {
            printf("spi read error\n");
            ret = ERR_IO;
            goto read_err;
        }

        hexdump8(data, len);

read_err:
        free(data);

    // spi spew <bus> <len> <iterations> <delay>
    } else if (IS_CMD("spew", 6)) {
        uint8_t bus = argv[2].i;
        size_t len = MIN(1024, argv[3].i);
        uint32_t cnt = argv[4].i;
        uint32_t delay = argv[5].i;
        uint8_t *data = (uint8_t *)malloc(len);

        if (cnt == 0) {
            cnt = UINT32_MAX;
        }

        for (uint32_t i = 0; i < len; i++) {
            data[i] = i % 255;
        }

        for (uint32_t i = 0; i < cnt; i++) {
            int spi_ret;
            if ((spi_ret = spi_write(&handles[bus], data, len, handle_nss[bus])) != NO_ERROR) {
                printf("spi write error: %d\n", spi_ret);
                ret = ERR_IO;
                goto spew_err;
            }

            if (delay > 0) {
                thread_sleep(delay);
            }
        }

spew_err:
        free(data);
    } else {
        print_spi_usage();
    }

    return ret;
}

STATIC_COMMAND_START
STATIC_COMMAND("spi", "spi commands for stm32f7", &cmd_spi)
STATIC_COMMAND_END(spi);



#endif // WITH_LIB_CONSOLE
