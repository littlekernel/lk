#pragma once
#include <err.h>
#include <compiler.h>
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

// Structure definitions for the STM32F7 spi controller. The intention is to point stm32f7_spi_t
// at one of the spi base addresses defines such as SPI1
//
// ex: volatile stm32f7_spi_t *spi2 = (stm32f7_spi_t *) SPI2;

typedef struct {
    uint16_t cpha:1;
    uint16_t cpol:1;
    uint16_t mstr:1;
    uint16_t br:3;
    uint16_t spe:1;
    uint16_t lsb_first:1;
    uint16_t ssi:1;
    uint16_t ssm:1;
    uint16_t rx_only:1;
    uint16_t crcl:1;
    uint16_t crc_next:1;
    uint16_t crc_en:1;
    uint16_t bidi_oe:1;
    uint16_t bidi_mode:1;
} SPIx_CR1_t;

typedef struct {
    uint16_t rxdmaen:1;
    uint16_t txdmaen:1;
    uint16_t ssoe:1;
    uint16_t nssp:1;
    uint16_t frf:1;
    uint16_t errie:1;
    uint16_t rxneie:1;
    uint16_t txeie:1;
    uint16_t ds:4;
    uint16_t frxth:1;
    uint16_t ldma_rx:1;
    uint16_t ldma_tx:1;
    uint16_t __reserved0:1;
} SPIx_CR2_t;

typedef struct {
    uint16_t rxne:1;
    uint16_t txe:1;
    uint16_t chside:1;
    uint16_t udr:1;
    uint16_t crcerr:1;
    uint16_t modf:1;
    uint16_t ovr:1;
    uint16_t bsy:1;
    uint16_t fre:1;
    uint16_t frlvl:2;
    uint16_t ftlvl:2;
    uint16_t __reserved0:3;
} SPIx_SR_t;

typedef struct {
    SPIx_CR1_t CR1;
    uint16_t __reserved0;
    SPIx_CR2_t CR2;
    uint16_t __reserved1;
    SPIx_SR_t SR;
    uint16_t __reserved2;
    uint16_t DR;
} stm32f7_spi_t;

enum cpha {
    cpha_first_transition  = 0x0,
    cpha_second_transition = 0x1,
};

enum cpol {
    cpol_clk_idle_low  = 0x0,
    cpol_clk_idle_high = 0x1,
};

enum mstr {
    mstr_spi_slave  = 0x0,
    mstr_spi_master = 0x1,
};

enum br {
    fpclk_div_2   = 0b000,
    fpclk_div_4   = 0b001,
    fpclk_div_8   = 0b010,
    fpclk_div_16  = 0b011,
    fpclk_div_32  = 0b100,
    fpclk_div_64  = 0b101,
    fpclk_div_128 = 0b110,
    fpclk_div_256 = 0b111,
};

enum spe {
    spe_spi_disable = 0x0,
    spe_spi_enabled = 0x1,
};

enum ssm {
    ssm_disabled = 0x0,
    ssm_enabled  = 0x1,
};

enum rxonly {
    rxonly_full_duplex     = 0x0,
    rxonly_output_disabled = 0x1,
};

enum crcl {
    crcl_8bit  = 0x0,
    crcl_16bit = 0x1,
};

enum crcnext {
    crcnext_tx_buf_next = 0x0,
    crcnext_tx_crc_next = 0x1,
};

enum crcen {
    hw_crc_disable = 0x0,
    hw_crc_enable  = 0x1,
};

enum bidioe {
    bidi_output_disable = 0x0,
    bidi_output_enable  = 0x1,
};

enum bidimode {
    bidi_mode_2 = 0x0,
    bidi_mode_1 = 0x1,
};

enum rxdmaen {
    rx_dma_disabled = 0x0,
    rx_dma_enabled  = 0x1,
};

enum txdmaen {
    tx_dma_disabled = 0x0,
    tx_dma_enabled  = 0x1,
};

enum ssoe {
    ss_output_disabled = 0x0,
    ss_output_enabled  = 0x1,
};

enum nssp {
    nss_pulse_disable = 0x0,
    nss_pulse_enable  = 0x1,
};

enum frf {
    motorola_mode = 0x0,
    ti_mode       = 0x1,
};

enum errie {
    err_int_mask   = 0x0,
    err_int_enable = 0x1,
};

enum rxneie {
    rxne_int_mask   = 0x0,
    rxne_int_enable = 0x1,
};

enum txeie {
    txe_int_mask   = 0x0,
    txe_int_enable = 0x1,
};

enum ds {
    data_size_4bit  = 0b0011,
    data_size_5bit  = 0b0100,
    data_size_6bit  = 0b0101,
    data_size_7bit  = 0b0110,
    data_size_8bit  = 0b0111,
    data_size_9bit  = 0b1000,
    data_size_10bit = 0b1001,
    data_size_11bit = 0b1010,
    data_size_12bit = 0b1011,
    data_size_13bit = 0b1100,
    data_size_14bit = 0b1101,
    data_size_15bit = 0b1110,
    data_size_16bit = 0b1111,
};

enum frxth {
    rxne_event_fifo_1_2 = 0x0,
    rxne_event_fifo_1_4 = 0x1,
};

enum ldma_rx {
    data_rx_even = 0x0,
    data_rx_odd  = 0x1,
};

enum ldma_tx {
    data_tx_even = 0x0,
    data_tx_odd  = 0x1,
};

enum sr_rxne {
    sr_rx_buf_empty     = 0x0,
    sr_rx_buf_not_empty = 0x1,
};

enum sr_txne {
    sr_tx_buf_empty     = 0x0,
    sr_tx_buf_not_empty = 0x1,
};

// CHSIDE unused in SPI mode
// UDR unused in SPI mode

enum crcerr {
    sr_crc_match    = 0x0,
    sr_crc_mismatch = 0x1,
};

enum modf {
    sr_mode_fault    = 0x0,
    sr_no_mode_fault = 0x1,
};

enum ovr {
    sr_no_overrun = 0x0,
    sr_overrun    = 0x1,
};

enum bsy {
    sr_not_busy   = 0x0,
    sr_busy       = 0x1,
};

enum fre {
    sr_no_fre = 0x0,
    sr_fre    = 0x1,
};

enum frlvl {
    sr_rx_fifo_empty = 0b00,
    sr_rx_fifo_1_4   = 0b01,
    sr_rx_fifo_1_2   = 0b10,
    sr_rx_fifo_full  = 0b11,
};

enum ftlvl {
    sr_tx_fifo_empty = 0b00,
    sr_tx_fifo_1_4   = 0b01,
    sr_tx_fifo_1_2   = 0b10,
    sr_tx_fifo_full  = 0b11,
};

STATIC_ASSERT(__builtin_offsetof(stm32f7_spi_t, CR1) == 0x0);
STATIC_ASSERT(__builtin_offsetof(stm32f7_spi_t, CR2) == 0x4);
STATIC_ASSERT(__builtin_offsetof(stm32f7_spi_t, SR) == 0x8);
STATIC_ASSERT(__builtin_offsetof(stm32f7_spi_t, DR) == 0xC);
