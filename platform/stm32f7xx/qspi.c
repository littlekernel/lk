#include <err.h>
#include <pow2.h>
#include <stdlib.h>
#include <string.h>

#include <lib/bio.h>
#include <platform/n25q128a.h>
#include <platform/qspi.h>

static QSPI_HandleTypeDef qspi_handle;
static const char device_name[] = "qspi-flash";
static bdev_t qspi_flash_device;
static bio_erase_geometry_info_t geometry;

// Functions exported to Block I/O handler.
static ssize_t spiflash_bdev_read(struct bdev* device, void* buf, off_t offset, size_t len);
static ssize_t spiflash_bdev_read_block(struct bdev* device, void* buf, bnum_t block, uint count);
static ssize_t spiflash_bdev_write_block(struct bdev* device, const void* buf, bnum_t block, uint count);
static ssize_t spiflash_bdev_erase(struct bdev* device, off_t offset, size_t len);
static int spiflash_ioctl(struct bdev* device, int request, void* argp);

static ssize_t qspi_write_page(uint32_t addr, const uint8_t *data);
static ssize_t qspi_erase_block(uint32_t block_addr);

status_t hal_error_to_status(HAL_StatusTypeDef hal_status);

static status_t qspi_write_enable(QSPI_HandleTypeDef* hqspi)
{
    QSPI_CommandTypeDef s_command;
    QSPI_AutoPollingTypeDef s_config;
    HAL_StatusTypeDef status;

    /* Enable write operations */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = WRITE_ENABLE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    status = HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    /* Configure automatic polling mode to wait for write enabling */
    s_config.Match = N25Q128A_SR_WREN;
    s_config.Mask = N25Q128A_SR_WREN;
    s_config.MatchMode = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval = 0x10;
    s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

    s_command.Instruction = READ_STATUS_REG_CMD;
    s_command.DataMode = QSPI_DATA_1_LINE;

    status = HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    return NO_ERROR;
}

static status_t qspi_dummy_cycles_cfg(QSPI_HandleTypeDef* hqspi)
{
    QSPI_CommandTypeDef s_command;
    uint8_t reg;
    HAL_StatusTypeDef status;

    /* Initialize the read volatile configuration register command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = READ_VOL_CFG_REG_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_1_LINE;
    s_command.DummyCycles = 0;
    s_command.NbData = 1;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    status = HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    /* Reception of the data */
    status = HAL_QSPI_Receive(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    /* Enable write operations */
    status = qspi_write_enable(hqspi);
    if (status != NO_ERROR) {
        return status;
    }

    /* Update volatile configuration register (with new dummy cycles) */
    s_command.Instruction = WRITE_VOL_CFG_REG_CMD;
    MODIFY_REG(
        reg, N25Q128A_VCR_NB_DUMMY,
        (N25Q128A_DUMMY_CYCLES_READ_QUAD << POSITION_VAL(N25Q128A_VCR_NB_DUMMY)));

    /* Configure the write volatile configuration register command */
    status = HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    /* Transmission of the data */
    status = HAL_QSPI_Transmit(hqspi, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    return NO_ERROR;
}

static status_t qspi_auto_polling_mem_ready(QSPI_HandleTypeDef* hqspi,
        uint32_t Timeout)
{
    QSPI_CommandTypeDef s_command;
    QSPI_AutoPollingTypeDef s_config;
    HAL_StatusTypeDef status;

    /* Configure automatic polling mode to wait for memory ready */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = READ_STATUS_REG_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_1_LINE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    s_config.Match = 0;
    s_config.Mask = N25Q128A_SR_WIP;
    s_config.MatchMode = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval = 0x10;
    s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

    status = HAL_QSPI_AutoPolling(hqspi, &s_command, &s_config, Timeout);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    return NO_ERROR;
}

static status_t qspi_reset_memory(QSPI_HandleTypeDef* hqspi)
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
        return hal_error_to_status(status);
    }

    /* Send the reset memory command */
    s_command.Instruction = RESET_MEMORY_CMD;
    status = HAL_QSPI_Command(hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    /* Configure automatic polling mode to wait the memory is ready */
    status = qspi_auto_polling_mem_ready(hqspi, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != NO_ERROR) {
        return hal_error_to_status(status);
    }

    return NO_ERROR;
}

static ssize_t spiflash_bdev_read(struct bdev* device, void* buf, off_t offset, size_t len)
{
    len = bio_trim_range(device, offset, len);
    if (len == 0) {
        return 0;
    }

    QSPI_CommandTypeDef s_command;
    HAL_StatusTypeDef status;

    // /* Initialize the read command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = QUAD_INOUT_FAST_READ_CMD;
    s_command.AddressMode = QSPI_ADDRESS_4_LINES;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_4_LINES;
    s_command.DummyCycles = N25Q128A_DUMMY_CYCLES_READ_QUAD;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    s_command.NbData = len;
    s_command.Address = offset;

    // /* Configure the command */
    status = HAL_QSPI_Command(&qspi_handle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    // /* Reception of the data */
    status = HAL_QSPI_Receive(&qspi_handle, buf, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    return len;
}

static ssize_t spiflash_bdev_read_block(struct bdev* device, void* buf,
                                        bnum_t block, uint count)
{
    count = bio_trim_block_range(device, block, count);
    if (count == 0)
        return 0;

    return spiflash_bdev_read(device, buf, block << device->block_shift,
                              count << device->block_shift);
}

static ssize_t spiflash_bdev_write_block(struct bdev* device, const void* _buf,
        bnum_t block, uint count)
{
    count = bio_trim_block_range(device, block, count);
    if (count == 0) {
        return 0;
    }

    const uint8_t *buf = _buf;

    ssize_t total_bytes_written = 0;
    for (; count > 0; count--, block++) {
        ssize_t bytes_written = qspi_write_page(block * N25Q128A_PAGE_SIZE, buf);
        if (bytes_written < 0) {
            return bytes_written;
        }

        buf += N25Q128A_PAGE_SIZE;
        total_bytes_written += bytes_written;
    }

    return total_bytes_written;
}

static ssize_t spiflash_bdev_erase(struct bdev* device, off_t offset,
                                   size_t len)
{
    len = bio_trim_range(device, offset, len);
    if (len == 0) {
        return 0;
    }

    ssize_t total_erased = 0;
    while (total_erased < (ssize_t)len) {
        ssize_t erased = qspi_erase_block(offset);
        if (erased < 0) {
            return erased;
        }
        total_erased += erased;
        offset += erased;
    }

    return total_erased;
}

static int spiflash_ioctl(struct bdev* device, int request, void* argp)
{
    return ERR_NOT_IMPLEMENTED;
}

static ssize_t qspi_write_page(uint32_t addr, const uint8_t *data)
{
    if (!IS_ALIGNED(addr, N25Q128A_PAGE_SIZE)) {
        return ERR_INVALID_ARGS;
    }

    HAL_StatusTypeDef status; 

    QSPI_CommandTypeDef s_command = {
        .InstructionMode   = QSPI_INSTRUCTION_1_LINE,
        .Instruction       = EXT_QUAD_IN_FAST_PROG_CMD,
        .AddressMode       = QSPI_ADDRESS_4_LINES,
        .AddressSize       = QSPI_ADDRESS_24_BITS,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE,
        .DataMode          = QSPI_DATA_4_LINES,
        .DummyCycles       = 0,
        .DdrMode           = QSPI_DDR_MODE_DISABLE,
        .DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY,
        .SIOOMode          = QSPI_SIOO_INST_EVERY_CMD,
        .Address           = addr,
        .NbData            = N25Q128A_PAGE_SIZE
    };

    status_t write_enable_result = qspi_write_enable(&qspi_handle);
    if (write_enable_result != NO_ERROR) {
        return write_enable_result;
    }

    status = HAL_QSPI_Command(&qspi_handle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    status = HAL_QSPI_Transmit(&qspi_handle, (uint8_t*)data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    status_t auto_polling_mem_ready_result = 
            qspi_auto_polling_mem_ready(&qspi_handle, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
    if (auto_polling_mem_ready_result != NO_ERROR) {
        return auto_polling_mem_ready_result;
    }

    return N25Q128A_PAGE_SIZE;
}

static ssize_t qspi_erase_block(uint32_t block_addr)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the erase command */
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction       = SUBSECTOR_ERASE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command.Address           = block_addr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    /* Enable write operations */
    status_t qspi_write_enable_result = qspi_write_enable(&qspi_handle);
    if (qspi_write_enable_result != NO_ERROR) {
        return qspi_write_enable_result;
    }

    /* Send the command */
    if (HAL_QSPI_Command(&qspi_handle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return ERR_GENERIC;
    }

    /* Configure automatic polling mode to wait for end of erase */
    status_t auto_polling_mem_ready_result = 
            qspi_auto_polling_mem_ready(&qspi_handle, N25Q128A_SUBSECTOR_ERASE_MAX_TIME);
    if (auto_polling_mem_ready_result != NO_ERROR) {
        return auto_polling_mem_ready_result;
    }

    return N25Q128A_SUBSECTOR_SIZE;
}


status_t qspi_flash_init(void)
{
    qspi_handle.Instance = QUADSPI;

    HAL_StatusTypeDef status;
    status_t result;

    status = HAL_QSPI_DeInit(&qspi_handle);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    // Setup the QSPI Flash device.
    qspi_handle.Init.ClockPrescaler = 1;
    qspi_handle.Init.FifoThreshold = 4;
    qspi_handle.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    qspi_handle.Init.FlashSize = POSITION_VAL(N25Q128A_FLASH_SIZE) - 1;
    qspi_handle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_2_CYCLE;
    qspi_handle.Init.ClockMode = QSPI_CLOCK_MODE_0;
    qspi_handle.Init.FlashID = QSPI_FLASH_ID_1;
    qspi_handle.Init.DualFlash = QSPI_DUALFLASH_DISABLE;

    status = HAL_QSPI_Init(&qspi_handle);
    if (status != HAL_OK) {
        return hal_error_to_status(status);
    }

    result = qspi_reset_memory(&qspi_handle);
    if (result != NO_ERROR) {
        return result;
    }

    result = qspi_dummy_cycles_cfg(&qspi_handle);
    if (result != NO_ERROR) {
        return result;
    }

    // Initialize the QSPI Flash and register it as a Block I/O device.
    geometry.erase_size = log2_uint(N25Q128A_SUBSECTOR_SIZE);
    geometry.erase_shift = log2_uint(N25Q128A_SUBSECTOR_SIZE);
    geometry.start = 0;
    geometry.size = N25Q128A_FLASH_SIZE;

    bio_initialize_bdev(&qspi_flash_device, device_name, N25Q128A_PAGE_SIZE,
                        (N25Q128A_FLASH_SIZE / N25Q128A_PAGE_SIZE), 1, &geometry);

    qspi_flash_device.read = &spiflash_bdev_read;
    qspi_flash_device.read_block = &spiflash_bdev_read_block;
    // qspi_flash_device.write has a default hook that will be okay
    qspi_flash_device.write_block = &spiflash_bdev_write_block;
    qspi_flash_device.erase = &spiflash_bdev_erase;
    qspi_flash_device.ioctl = &spiflash_ioctl;

    bio_register_device(&qspi_flash_device);

    return NO_ERROR;
}

status_t hal_error_to_status(HAL_StatusTypeDef hal_status) 
{
    switch(hal_status) {
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