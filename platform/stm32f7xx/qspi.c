/*
 * Copyright (c) 2015 Gurjant Kalsi <me@gurjantkalsi.com>
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
#include <pow2.h>
#include <stdlib.h>
#include <string.h>

#include <arch/arm/cm.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <lib/bio.h>
#include <platform.h>
#include <platform/n25qxxa.h>
#include <platform/n25q128a.h>
#include <platform/n25q512a.h>
#include <platform/qspi.h>
#include <trace.h>

#define LOCAL_TRACE 0

#define FOUR_BYTE_ADDR_THRESHOLD (1 << 24)
#define LOCAL_TRACE 0
#define MAX_DMA_WAIT_MS 1024

typedef void (*CpltCallback)(void);

typedef enum {
    QSPI_STATE_LINEAR,
    QSPI_STATE_COMMAND,
    QSPI_STATE_MAX
} device_state_t;
device_state_t device_state;


static QSPI_HandleTypeDef qspi_handle;
static DMA_Stream_TypeDef *dma2_stream7;
static CpltCallback cplt_callback;

static const char device_name[] = "qspi-flash";
static bdev_t qspi_flash_device;
static bio_erase_geometry_info_t geometry;

static mutex_t spiflash_mutex;

// Functions exported to Block I/O handler.
static ssize_t spiflash_bdev_read(struct bdev *device, void *buf, off_t offset, size_t len);
static ssize_t spiflash_bdev_read_block(struct bdev *device, void *buf, bnum_t block, uint count);
static ssize_t spiflash_bdev_write_block(struct bdev *device, const void *buf, bnum_t block, uint count);
static ssize_t spiflash_bdev_erase(struct bdev *device, off_t offset, size_t len);
static int spiflash_ioctl(struct bdev *device, int request, void *argp);

static ssize_t qspi_write_page_unsafe(uint32_t addr, const uint8_t *data);

static ssize_t qspi_erase(bdev_t *device, uint32_t block_addr, uint32_t instruction);
static ssize_t qspi_bulk_erase(bdev_t *device);
static ssize_t qspi_erase_sector(bdev_t *device, uint32_t block_addr);
static ssize_t qspi_erase_subsector(bdev_t *device, uint32_t block_addr);
static status_t qspi_auto_polling_mem_ready_unsafe(QSPI_HandleTypeDef *hqspi, uint8_t match, uint8_t mask);

static HAL_StatusTypeDef qspi_cmd(QSPI_HandleTypeDef *, QSPI_CommandTypeDef *);
static HAL_StatusTypeDef qspi_tx_dma(QSPI_HandleTypeDef *, QSPI_CommandTypeDef *, uint8_t *);
static HAL_StatusTypeDef qspi_rx_dma(QSPI_HandleTypeDef *, QSPI_CommandTypeDef *, uint8_t *);

static status_t qspi_enable_linear(void);
static status_t qspi_disable_linear(void);
static bool qspi_is_linear(void);

status_t qspi_dma_init(QSPI_HandleTypeDef *hqspi);

static uint32_t get_specialized_instruction(uint32_t instruction, uint32_t address);
static uint32_t get_address_size(uint32_t address);

static event_t cmd_event;
static event_t rx_event;
static event_t tx_event;
static event_t st_event;

status_t hal_error_to_status(HAL_StatusTypeDef hal_status);

// Unsetting the DMA Enable bit in the DMA Control register isn't enough to
// disable the DMA Engine since DMA transfers may still be in progress.
// We have to wait for the DMA Engine to acknowledge being disabled by watching
// the DMA Enable bit.
static status_t dma_disable(DMA_Stream_TypeDef *dma)
{
    // Unset the DMA Enable bit.
    dma->CR &= ~DMA_SxCR_EN;

    lk_time_t start_time = current_time();

    while (dma->CR & DMA_SxCR_EN) {

        dma->CR &= ~DMA_SxCR_EN;

        if (current_time() - start_time > MAX_DMA_WAIT_MS) {
            return ERR_TIMED_OUT;
        }
    }

    return NO_ERROR;
}

// Must hold spiflash_mutex before calling.
static status_t qspi_write_enable_unsafe(QSPI_HandleTypeDef *hqspi)
{
    HAL_StatusTypeDef status;

    static const QSPI_CommandTypeDef s_command = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = WRITE_ENABLE_CMD,
        .AddressMode = QSPI_ADDRESS_NONE,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode = QSPI_DATA_NONE,
        .DummyCycles = 0,
        .DdrMode = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode = QSPI_SIOO_INST_EVERY_CMD
    };

    status = HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Command failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }

    status = qspi_auto_polling_mem_ready_unsafe(hqspi, N25QXXA_SR_WREN, N25QXXA_SR_WREN);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: auto_polling_mem_ready failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }

    return NO_ERROR;
}

// Must hold spiflash_mutex before calling.
static status_t qspi_dummy_cycles_cfg_unsafe(QSPI_HandleTypeDef *hqspi)
{
    uint8_t reg;
    HAL_StatusTypeDef status;

    /* Initialize the read volatile configuration register command */
    static const QSPI_CommandTypeDef init_rvcr_cmd = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = READ_VOL_CFG_REG_CMD,
        .AddressMode = QSPI_ADDRESS_NONE,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode = QSPI_DATA_1_LINE,
        .DummyCycles = 0,
        .NbData = 1,
        .DdrMode = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode = QSPI_SIOO_INST_EVERY_CMD
    };

    /* Configure the command */
    status = HAL_QSPI_Command(hqspi, &init_rvcr_cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Command(init_rvcr_cmd) failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }

    /* Reception of the data */
    status = HAL_QSPI_Receive(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Receive failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }

    /* Enable write operations */
    status = qspi_write_enable_unsafe(hqspi);
    if (status != NO_ERROR) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Receive failed with err = %d\n",
                __func__, status);
        return status;
    }

    /* Update volatile configuration register (with new dummy cycles) */
    static const QSPI_CommandTypeDef update_rvcr_cmd = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = WRITE_VOL_CFG_REG_CMD,
        .AddressMode = QSPI_ADDRESS_NONE,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode = QSPI_DATA_1_LINE,
        .DummyCycles = 0,
        .NbData = 1,
        .DdrMode = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode = QSPI_SIOO_INST_EVERY_CMD
    };
    MODIFY_REG(
        reg, N25QXXA_VCR_NB_DUMMY,
        (N25QXXA_DUMMY_CYCLES_READ_QUAD << POSITION_VAL(N25QXXA_VCR_NB_DUMMY)));

    /* Configure the write volatile configuration register command */
    status = HAL_QSPI_Command(hqspi, &update_rvcr_cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Command(update_rvcr_cmd) failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }

    /* Transmission of the data */
    status = HAL_QSPI_Transmit(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Transmit failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }

    return NO_ERROR;
}

// Must hold spiflash_mutex before calling.
static status_t qspi_auto_polling_mem_ready_unsafe(QSPI_HandleTypeDef *hqspi, uint8_t match, uint8_t mask)
{
    QSPI_AutoPollingTypeDef s_config;
    HAL_StatusTypeDef status;

    static const QSPI_CommandTypeDef s_command = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = READ_STATUS_REG_CMD,
        .AddressMode = QSPI_ADDRESS_NONE,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode = QSPI_DATA_1_LINE,
        .DummyCycles = 0,
        .DdrMode = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode = QSPI_SIOO_INST_EVERY_CMD,
        .NbData = 1
    };

    s_config.Match = match;
    s_config.Mask = mask;
    s_config.MatchMode = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval = 0x10;
    s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

    status = HAL_QSPI_AutoPolling_IT(hqspi, &s_command, &s_config);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: HAL_QSPI_AutoPolling_IT failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }
    event_wait(&st_event);

    return NO_ERROR;
}

// Must hold spiflash_mutex before calling.
static status_t qspi_reset_memory_unsafe(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef s_command;
    HAL_StatusTypeDef status;

    /* Initialize the reset enable command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = RESET_ENABLE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    status = HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Command(RESET_ENABLE_CMD) failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }

    /* Send the reset memory command */
    s_command.Instruction = RESET_MEMORY_CMD;
    status = HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Command(RESET_MEMORY_CMD) failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }

    /* Configure automatic polling mode to wait the memory is ready */
    status = qspi_auto_polling_mem_ready_unsafe(hqspi, 0, N25QXXA_SR_WIP);
    if (status != NO_ERROR) {
        dprintf(CRITICAL, "%s: auto_polling_mem_ready failed with err = %d\n",
                __func__, status);
        return hal_error_to_status(status);
    }

    return NO_ERROR;
}

static ssize_t spiflash_bdev_read_block(struct bdev *device, void *buf,
                                        bnum_t block, uint count)
{
    LTRACEF("device %p, buf %p, block %u, count %u\n",
            device, buf, block, count);

    if (!IS_ALIGNED((uintptr_t)buf, CACHE_LINE)) {
        DEBUG_ASSERT(IS_ALIGNED((uintptr_t)buf, CACHE_LINE));
        return ERR_INVALID_ARGS;
    }

    count = bio_trim_block_range(device, block, count);
    if (count == 0)
        return 0;

    QSPI_CommandTypeDef s_command;
    HAL_StatusTypeDef status;

    uint64_t largest_offset = (block + count) * device->block_size - 1;

    // /* Initialize the read command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = get_specialized_instruction(QUAD_OUT_FAST_READ_CMD, largest_offset);
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize = get_address_size(largest_offset);
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_4_LINES;
    s_command.DummyCycles = N25QXXA_DUMMY_CYCLES_READ_QUAD;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    s_command.NbData = device->block_size;

    ssize_t retcode = 0;

    mutex_acquire(&spiflash_mutex);

    s_command.Address = block * device->block_size;
    for (uint i = 0; i < count; i++) {

        status = HAL_QSPI_Command(&qspi_handle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
        if (status != HAL_OK) {
            retcode = hal_error_to_status(status);
            dprintf(CRITICAL, "%s: HAL_QSPI_Command failed with err = %ld\n",
                    __func__, retcode);
            goto err;
        }

        // /* Reception of the data */
        status = qspi_rx_dma(&qspi_handle, &s_command, buf);
        if (status != HAL_OK) {
            retcode = hal_error_to_status(status);
            dprintf(CRITICAL, "%s: qspi_rx_dma failed with err = %ld\n",
                    __func__, retcode);
            goto err;
        }

        buf += device->block_size;
        retcode += device->block_size;
        s_command.Address += device->block_size;
    }

err:
    mutex_release(&spiflash_mutex);
    return retcode;
}

static ssize_t spiflash_bdev_write_block(struct bdev *device, const void *_buf,
        bnum_t block, uint count)
{
    count = bio_trim_block_range(device, block, count);
    if (count == 0) {
        return 0;
    }

    const uint8_t *buf = _buf;

    mutex_acquire(&spiflash_mutex);

    ssize_t total_bytes_written = 0;
    for (; count > 0; count--, block++) {
        ssize_t bytes_written = qspi_write_page_unsafe(block * N25QXXA_PAGE_SIZE, buf);
        if (bytes_written < 0) {
            dprintf(CRITICAL, "%s: qspi_write_page_unsafe failed with err = %ld\n",
                    __func__, bytes_written);
            total_bytes_written = bytes_written;
            goto err;
        }

        buf += N25QXXA_PAGE_SIZE;
        total_bytes_written += bytes_written;
    }

err:
    mutex_release(&spiflash_mutex);
    return total_bytes_written;
}

static ssize_t spiflash_bdev_erase(struct bdev *device, off_t offset,
                                   size_t len)
{
    len = bio_trim_range(device, offset, len);
    if (len == 0) {
        return 0;
    }

    ssize_t total_erased = 0;

    mutex_acquire(&spiflash_mutex);

    // Choose an erase strategy based on the number of bytes being erased.
    if (len == device->total_size && offset == 0) {
        // Bulk erase the whole flash.
        total_erased = qspi_bulk_erase(device);
        goto finish;
    }

    // Erase as many sectors as necessary, then switch to subsector erase for
    // more fine grained erasure.
    while (((ssize_t)len - total_erased) >= N25QXXA_SECTOR_SIZE) {
        ssize_t erased = qspi_erase_sector(device, offset);
        if (erased < 0) {
            total_erased = erased;
            goto finish;
        }
        total_erased += erased;
        offset += erased;
    }

    while (total_erased < (ssize_t)len) {
        ssize_t erased = qspi_erase_subsector(device, offset);
        if (erased < 0) {
            total_erased = erased;
            goto finish;
        }
        total_erased += erased;
        offset += erased;
    }

finish:
    mutex_release(&spiflash_mutex);
    return total_erased;
}

static int spiflash_ioctl(struct bdev *device, int request, void *argp)
{
    int ret = NO_ERROR;

    switch (request) {
        case BIO_IOCTL_GET_MEM_MAP:
            /* put the device into linear mode */
            ret = qspi_enable_linear();
            // Fallthrough.
        case BIO_IOCTL_GET_MAP_ADDR:
            if (argp)
                *(void **)argp = (void *)QSPI_BASE;
            break;
        case BIO_IOCTL_PUT_MEM_MAP:
            ret = qspi_disable_linear();
            break;
        case BIO_IOCTL_IS_MAPPED:
            if (argp)
                *(void **)argp = (void *)qspi_is_linear();
            break;
        default:
            ret = ERR_NOT_SUPPORTED;
    }

    return ret;
}

static ssize_t qspi_write_page_unsafe(uint32_t addr, const uint8_t *data)
{
    if (!IS_ALIGNED(addr, N25QXXA_PAGE_SIZE)) {
        return ERR_INVALID_ARGS;
    }

    HAL_StatusTypeDef status;

    QSPI_CommandTypeDef s_command = {
        .InstructionMode   = QSPI_INSTRUCTION_1_LINE,
        .Instruction       = get_specialized_instruction(QUAD_IN_FAST_PROG_CMD, addr),
        .AddressMode       = QSPI_ADDRESS_1_LINE,
        .AddressSize       = get_address_size(addr),
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode          = QSPI_DATA_4_LINES,
        .DummyCycles       = 0,
        .DdrMode           = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD,
        .Address           = addr,
        .NbData            = N25QXXA_PAGE_SIZE
    };

    status_t write_enable_result = qspi_write_enable_unsafe(&qspi_handle);
    if (write_enable_result != NO_ERROR) {
        dprintf(CRITICAL, "%s: qspi_write_enable_unsafe failed with err = %d\n",
                    __func__, write_enable_result);
        return write_enable_result;
    }

    status = HAL_QSPI_Command(&qspi_handle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Command failed with err = %d\n",
                    __func__, status);
        return hal_error_to_status(status);
    }

    status = qspi_tx_dma(&qspi_handle, &s_command, (uint8_t *)data);
    if (status != HAL_OK) {
        dprintf(CRITICAL, "%s: qspi_tx_dma failed with err = %d\n",
                    __func__, status);
        return hal_error_to_status(status);
    }

    status_t auto_polling_mem_ready_result =
        qspi_auto_polling_mem_ready_unsafe(&qspi_handle, 0, N25QXXA_SR_WIP);
    if (auto_polling_mem_ready_result != NO_ERROR) {
        dprintf(CRITICAL, "%s: auto_polling_mem_ready failed with err = %d\n",
                __func__, auto_polling_mem_ready_result);
        return auto_polling_mem_ready_result;
    }

    return N25QXXA_PAGE_SIZE;
}


status_t qspi_flash_init(size_t flash_size)
{
    status_t result = NO_ERROR;

    event_init(&cmd_event, false, EVENT_FLAG_AUTOUNSIGNAL);
    event_init(&tx_event, false, EVENT_FLAG_AUTOUNSIGNAL);
    event_init(&rx_event, false, EVENT_FLAG_AUTOUNSIGNAL);
    event_init(&st_event, false, EVENT_FLAG_AUTOUNSIGNAL);

    mutex_init(&spiflash_mutex);
    result = mutex_acquire(&spiflash_mutex);
    if (result != NO_ERROR) {
        return result;
    }

    qspi_handle.Instance = QUADSPI;

    HAL_StatusTypeDef status;

    // Enable the QuadSPI memory interface clock
    __HAL_RCC_QSPI_CLK_ENABLE();

    // Reset the QuadSPI memory interface
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();

    // Setup the QSPI Flash device.
    qspi_handle.Init.ClockPrescaler = 1;
    qspi_handle.Init.FifoThreshold = 4;
    qspi_handle.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    qspi_handle.Init.FlashSize = POSITION_VAL(flash_size) - 1;
    qspi_handle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
    qspi_handle.Init.ClockMode = QSPI_CLOCK_MODE_0;
    qspi_handle.Init.FlashID = QSPI_FLASH_ID_1;
    qspi_handle.Init.DualFlash = QSPI_DUALFLASH_DISABLE;

    status = HAL_QSPI_Init(&qspi_handle);
    if (status != HAL_OK) {
        result = hal_error_to_status(status);
        dprintf(CRITICAL, "%s: HAL_QSPI_Init failed with err = %d\n",
                __func__, result);
        goto err;
    }

    // enable the qspi interrupt
    HAL_NVIC_EnableIRQ(QUADSPI_IRQn);

    result = qspi_reset_memory_unsafe(&qspi_handle);
    if (result != NO_ERROR) {
        dprintf(CRITICAL, "%s: qspi_reset_memory_unsafe failed with err = %d\n",
                __func__, result);
        goto err;
    }

    result = qspi_dummy_cycles_cfg_unsafe(&qspi_handle);
    if (result != NO_ERROR) {
        dprintf(CRITICAL, "%s: qspi_dummy_cycles_cfg_unsafe failed with err = %d\n",
                __func__, result);
        goto err;
    }

    result = qspi_dma_init(&qspi_handle);
    if (result != NO_ERROR) {
        dprintf(CRITICAL, "%s: qspi_dma_init failed with err = %d\n",
                __func__, result);
        goto err;
    }

    result = hal_error_to_status(HAL_QSPI_Abort(&qspi_handle));
    if (result != NO_ERROR) {
        dprintf(CRITICAL, "%s: HAL_QSPI_Abort failed with err = %d\n",
                __func__, result);
        goto err;
    }
    device_state = QSPI_STATE_COMMAND;

    // Initialize the QSPI Flash and register it as a Block I/O device.
    geometry.erase_size = log2_uint(N25QXXA_SUBSECTOR_SIZE);
    geometry.erase_shift = log2_uint(N25QXXA_SUBSECTOR_SIZE);
    geometry.start = 0;
    geometry.size = flash_size;

    bio_initialize_bdev(&qspi_flash_device, device_name, N25QXXA_PAGE_SIZE,
                        (flash_size / N25QXXA_PAGE_SIZE), 1, &geometry,
                        BIO_FLAG_CACHE_ALIGNED_READS);

    // qspi_flash_device.read: Use default hook.
    qspi_flash_device.read_block = &spiflash_bdev_read_block;
    // qspi_flash_device.write has a default hook that will be okay
    qspi_flash_device.write_block = &spiflash_bdev_write_block;
    qspi_flash_device.erase = &spiflash_bdev_erase;
    qspi_flash_device.ioctl = &spiflash_ioctl;

    /* we erase to 0xff */
    qspi_flash_device.erase_byte = 0xff;

    bio_register_device(&qspi_flash_device);

err:
    mutex_release(&spiflash_mutex);
    return result;
}

status_t hal_error_to_status(HAL_StatusTypeDef hal_status)
{
    switch (hal_status) {
        case HAL_OK:
            return NO_ERROR;
        case HAL_ERROR:
            return ERR_GENERIC;
        case HAL_BUSY:
            return ERR_BUSY;
        case HAL_TIMEOUT:
            return ERR_TIMED_OUT;
        default:
            return ERR_GENERIC;
    }
}

static ssize_t qspi_erase(bdev_t *device, uint32_t block_addr, uint32_t instruction)
{
    if (instruction == BULK_ERASE_CMD && block_addr != 0) {
        // This call was probably not what the user intended since the
        // block_addr is irrelevant when performing a bulk erase.
        return ERR_INVALID_ARGS;
    }

    QSPI_CommandTypeDef erase_cmd;
    ssize_t num_erased_bytes;
    switch (instruction) {
        case SUBSECTOR_ERASE_CMD: {
            num_erased_bytes = N25QXXA_SUBSECTOR_SIZE;
            erase_cmd.AddressSize = get_address_size(block_addr);
            erase_cmd.Instruction = get_specialized_instruction(instruction, block_addr);
            erase_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
            erase_cmd.Address     = block_addr;

            break;
        }
        case SECTOR_ERASE_CMD: {
            num_erased_bytes = N25QXXA_SECTOR_SIZE;
            erase_cmd.AddressSize = get_address_size(block_addr);
            erase_cmd.Instruction = get_specialized_instruction(instruction, block_addr);
            erase_cmd.AddressMode = QSPI_ADDRESS_1_LINE;
            erase_cmd.Address     = block_addr;

            break;
        }
        case BULK_ERASE_CMD: {
            num_erased_bytes = device->total_size;
            erase_cmd.AddressMode = QSPI_ADDRESS_NONE;
            erase_cmd.Instruction = instruction;
            break;
        }
        default: {
            // Instruction must be a valid erase instruction.
            return ERR_INVALID_ARGS;
        }
    }

    erase_cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    erase_cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    erase_cmd.DataMode          = QSPI_DATA_NONE;
    erase_cmd.DummyCycles       = 0;
    erase_cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    erase_cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    erase_cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;


    /* Enable write operations */
    status_t qspi_write_enable_result = qspi_write_enable_unsafe(&qspi_handle);
    if (qspi_write_enable_result != NO_ERROR) {
        dprintf(CRITICAL, "%s: qspi_write_enable_unsafe failed with err = %d\n",
                __func__, qspi_write_enable_result);
        return qspi_write_enable_result;
    }

    /* Send the command */
    if (HAL_QSPI_Command(&qspi_handle, &erase_cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return ERR_GENERIC;
    }

    /* Configure automatic polling mode to wait for end of erase */
    status_t auto_polling_mem_ready_result =
        qspi_auto_polling_mem_ready_unsafe(&qspi_handle, 0, N25QXXA_SR_WIP);
    if (auto_polling_mem_ready_result != NO_ERROR) {
        dprintf(CRITICAL, "%s: auto_polling_mem_ready failed with err = %d\n",
                __func__, auto_polling_mem_ready_result);
        return auto_polling_mem_ready_result;
    }

    return num_erased_bytes;
}

static ssize_t qspi_bulk_erase(bdev_t *device)
{
    return qspi_erase(device, 0, BULK_ERASE_CMD);
}

static ssize_t qspi_erase_sector(bdev_t *device, uint32_t block_addr)
{
    return qspi_erase(device, block_addr, SECTOR_ERASE_CMD);
}

static ssize_t qspi_erase_subsector(bdev_t *device, uint32_t block_addr)
{
    return qspi_erase(device, block_addr, SUBSECTOR_ERASE_CMD);
}

static HAL_StatusTypeDef qspi_cmd(QSPI_HandleTypeDef *qspi_handle,
                                  QSPI_CommandTypeDef *s_command)
{
    HAL_StatusTypeDef result = HAL_QSPI_Command_IT(qspi_handle, s_command);

    if (result != HAL_OK) {
        return result;
    }

    event_wait(&cmd_event);
    return result;
}


static void setup_dma(DMA_Stream_TypeDef *stream, uint32_t peripheral_address,
                      uint32_t memory_address, uint32_t num_bytes,
                      uint32_t direction)
{
    stream->PAR = peripheral_address;
    stream->M0AR = memory_address;
    stream->NDTR = num_bytes;

    uint32_t dma_cr = 0;

    // Select Channel 3
    dma_cr |= DMA_CHANNEL_3;

    // Set the transfer priority.
    dma_cr |= DMA_SxCR_PL;

    // Enable auto memory pointer increment.
    dma_cr |= DMA_SxCR_MINC;

    if (direction == DMA_MEMORY_TO_PERIPH) {
        dma_cr |= DMA_SxCR_DIR_0;
    }

    // Turn on transfer complete and error interrupts.
    dma_cr |= DMA_SxCR_TCIE;
    dma_cr |= DMA_SxCR_TEIE;
    dma_cr |= DMA_SxCR_DMEIE;

    stream->CR = dma_cr;
}

/* IRQ Context */
void DMA_RxCpltCallback(void)
{
    event_signal(&rx_event, false);
}

/* IRQ Context */
void DMA_TxCpltCallback(void)
{
    event_signal(&tx_event, false);
}

/* IRQ Context */
void DMA_ErrorCallback(void)
{
    printf("DMA Error\n");
}

// Send data and wait for interrupt.
static HAL_StatusTypeDef qspi_tx_dma(QSPI_HandleTypeDef *qspi_handle, QSPI_CommandTypeDef *s_command, uint8_t *buf)
{
    MODIFY_REG(qspi_handle->Instance->CCR, QUADSPI_CCR_FMODE, 0);

    if (dma_disable(dma2_stream7) != NO_ERROR) {
        dprintf(CRITICAL, "%s: timed out while waiting for DMA to disable.\n", __func__);
        return ERR_TIMED_OUT;
    }

    setup_dma(
        dma2_stream7,
        (uint32_t)&(qspi_handle->Instance->DR),
        (uint32_t)buf,
        s_command->NbData,
        DMA_MEMORY_TO_PERIPH
    );

    // Make sure cache is flushed to RAM before invoking the DMA controller.
    arch_clean_cache_range((addr_t)buf, s_command->NbData);

    cplt_callback = DMA_TxCpltCallback;

    // And we're off to the races...
    dma2_stream7->CR |= DMA_SxCR_EN;
    qspi_handle->Instance->CR |= QUADSPI_CR_DMAEN;

    event_wait(&tx_event);

    return HAL_OK;
}

// Send data and wait for interrupt.
static HAL_StatusTypeDef qspi_rx_dma(QSPI_HandleTypeDef *qspi_handle, QSPI_CommandTypeDef *s_command, uint8_t *buf)
{
    // Make sure the front and back of the buffer are cache aligned.
    DEBUG_ASSERT(IS_ALIGNED((uintptr_t)buf, CACHE_LINE));
    DEBUG_ASSERT(IS_ALIGNED(((uintptr_t)buf) + s_command->NbData, CACHE_LINE));

    MODIFY_REG(qspi_handle->Instance->CCR, QUADSPI_CCR_FMODE, QUADSPI_CCR_FMODE_0);

    if (dma_disable(dma2_stream7) != NO_ERROR) {
        dprintf(CRITICAL, "%s: timed out while waiting for DMA to disable.\n", __func__);
        return ERR_TIMED_OUT;
    }

    setup_dma(
        dma2_stream7,
        (uint32_t)&(qspi_handle->Instance->DR),
        (uint32_t)buf,
        s_command->NbData,
        DMA_PERIPH_TO_MEMORY
    );

    cplt_callback = DMA_RxCpltCallback;

    arch_invalidate_cache_range((addr_t)buf, s_command->NbData);

    // And we're off to the races...
    dma2_stream7->CR |= DMA_SxCR_EN;
    uint32_t addr_reg = qspi_handle->Instance->AR;
    qspi_handle->Instance->AR = addr_reg;
    qspi_handle->Instance->CR |= QUADSPI_CR_DMAEN;

    event_wait(&rx_event);

    return HAL_OK;
}

void stm32_QUADSPI_IRQ(void)
{
    arm_cm_irq_entry();
    HAL_QSPI_IRQHandler(&qspi_handle);
    arm_cm_irq_exit(true);
}

void stm32_DMA2_Stream7_IRQ(void)
{
    arm_cm_irq_entry();

    // Make a copy of the interrupts that we're handling.
    uint32_t hisr = DMA2->HISR;

    // Xfer Complete?
    if (hisr & DMA_FLAG_TCIF3_7) {
        DMA2->HIFCR |= DMA_FLAG_TCIF3_7;

        qspi_handle.Instance->CR &= ~QUADSPI_CR_DMAEN;

        dma_disable(dma2_stream7);

        __HAL_QSPI_CLEAR_FLAG((&qspi_handle), QSPI_FLAG_TC);

        HAL_QSPI_Abort(&qspi_handle);
        qspi_handle.State = HAL_QSPI_STATE_READY;

        cplt_callback();
    }

    // Xfer Error?
    if (hisr & DMA_FLAG_TEIF3_7) {
        DMA2->HIFCR |= DMA_FLAG_TEIF3_7;
        DMA_ErrorCallback();
    }

    // Direct mode error?
    if (hisr & DMA_FLAG_DMEIF3_7) {
        DMA2->HIFCR |= DMA_FLAG_DMEIF3_7;
        DMA_ErrorCallback();
    }

    arm_cm_irq_exit(true);
}

/* IRQ Context */
void HAL_QSPI_CmdCpltCallback(QSPI_HandleTypeDef *hqspi)
{
    event_signal(&cmd_event, false);
}

/* IRQ Context */
void HAL_QSPI_StatusMatchCallback(QSPI_HandleTypeDef *hqspi)
{
    event_signal(&st_event, false);
}

/* IRQ Context */
void HAL_QSPI_ErrorCallback(QSPI_HandleTypeDef *hqspi)
{
    dprintf(CRITICAL, "%s: HAL QSPI Error.\n", __func__);
}

status_t qspi_dma_init(QSPI_HandleTypeDef *hqspi)
{
    /* QSPI DMA Controller Clock */
    __HAL_RCC_DMA2_CLK_ENABLE();

    dma2_stream7 = DMA2_Stream7;

    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    return NO_ERROR;
}

static uint32_t get_address_size(uint32_t address)
{
    if (address >= FOUR_BYTE_ADDR_THRESHOLD) {
        return QSPI_ADDRESS_32_BITS;
    }
    return QSPI_ADDRESS_24_BITS;
}

// Converts a 3 byte instruction into a 4 byte instruction if necessary.
static uint32_t get_specialized_instruction(uint32_t instruction, uint32_t address)
{
    if (address < FOUR_BYTE_ADDR_THRESHOLD) {
        return instruction;
    }

    switch (instruction) {
        case READ_CMD:
            return READ_4_BYTE_ADDR_CMD;
        case FAST_READ_CMD:
            return FAST_READ_4_BYTE_ADDR_CMD;
        case DUAL_OUT_FAST_READ_CMD:
            return DUAL_OUT_FAST_READ_4_BYTE_ADDR_CMD;
        case DUAL_INOUT_FAST_READ_CMD:
            return DUAL_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
        case QUAD_OUT_FAST_READ_CMD:
            return QUAD_OUT_FAST_READ_4_BYTE_ADDR_CMD;
        case QUAD_INOUT_FAST_READ_CMD:
            return QUAD_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
        case PAGE_PROG_CMD:
            return PAGE_PROG_4_BYTE_ADDR_CMD;
        case QUAD_IN_FAST_PROG_CMD:
            return QUAD_IN_FAST_PROG_4_BYTE_ADDR_CMD;
        case SUBSECTOR_ERASE_CMD:
            return SUBSECTOR_ERASE_4_BYTE_ADDR_CMD;
        case SECTOR_ERASE_CMD:
            return SECTOR_ERASE_4_BYTE_ADDR_CMD;
    }

    return instruction;
}

static status_t qspi_enable_linear(void)
{
    status_t result = NO_ERROR;

    mutex_acquire(&spiflash_mutex);

    if (device_state == QSPI_STATE_LINEAR) {
        // Device is already in linear mode, nothing to be done.
        goto finish;
    }

    result = qspi_dummy_cycles_cfg_unsafe(&qspi_handle);

    static const QSPI_CommandTypeDef s_command = {
        .InstructionMode   = QSPI_INSTRUCTION_1_LINE,
        .AddressSize       = QSPI_ADDRESS_24_BITS,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DdrMode           = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
        .AddressMode       = QSPI_ADDRESS_1_LINE,
        .Instruction       = QUAD_OUT_FAST_READ_CMD,
        .DataMode          = QSPI_DATA_4_LINES,
        .DummyCycles       = 10,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD
    };

    QSPI_MemoryMappedTypeDef linear_mode_cfg = {
        .TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE,
    };

    HAL_StatusTypeDef hal_result = HAL_QSPI_MemoryMapped(&qspi_handle, &s_command, &linear_mode_cfg);
    if (hal_result != HAL_OK) {
        result = hal_error_to_status(hal_result);
        dprintf(CRITICAL, "%s: HAL_QSPI_MemoryMapped failed with err = %d\n",
                __func__, hal_result);
        goto finish;
    }

    device_state = QSPI_STATE_LINEAR;

finish:
    mutex_release(&spiflash_mutex);
    return result;
}


static status_t qspi_disable_linear(void)
{
    status_t result = NO_ERROR;

    mutex_acquire(&spiflash_mutex);

    if (device_state == QSPI_STATE_COMMAND) {
        // Device is already in Command mode, nothing to be done.
        goto finish;
    }

    result = hal_error_to_status(HAL_QSPI_Abort(&qspi_handle));
    if (result == NO_ERROR) {
        device_state = QSPI_STATE_COMMAND;
    } else {
        dprintf(CRITICAL, "%s: HAL_QSPI_Abort failed with err = %d\n",
                __func__, result);
    }


finish:
    mutex_release(&spiflash_mutex);
    return result;
}

static bool qspi_is_linear(void)
{
    bool result;
    mutex_acquire(&spiflash_mutex);
    result = (QSPI_STATE_LINEAR == device_state);
    mutex_release(&spiflash_mutex);
    return result;
}