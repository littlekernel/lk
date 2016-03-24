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
/*
 * COPYRIGHT(c) 2015 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include <err.h>
#include <debug.h>
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <dev/gpio.h>
#include <platform/stm32.h>
#include <platform/sdram.h>

/*
 * sdram initialization sequence, taken from
 * STM32Cube_FW_F7_V1.1.0/Drivers/BSP
 */

/**
  * @brief  SDRAM status structure definition
  */
#define   SDRAM_OK         ((uint8_t)0x00)
#define   SDRAM_ERROR      ((uint8_t)0x01)

/* SDRAM refresh counter (100Mhz SD clock) */
#define REFRESH_COUNT      ((uint32_t)0x0603)

#define SDRAM_TIMEOUT      ((uint32_t)0xFFFF)

/* DMA definitions for SDRAM DMA transfer */
#define __DMAx_CLK_ENABLE                 __HAL_RCC_DMA2_CLK_ENABLE
#define __DMAx_CLK_DISABLE                __HAL_RCC_DMA2_CLK_DISABLE
#define SDRAM_DMAx_CHANNEL                DMA_CHANNEL_0
#define SDRAM_DMAx_STREAM                 DMA2_Stream0
#define SDRAM_DMAx_IRQn                   DMA2_Stream0_IRQn
#define SDRAM_DMAx_IRQHandler             DMA2_Stream0_IRQHandler

/**
  * @brief  FMC SDRAM Mode definition register defines
  */
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_1              ((uint16_t)0x0010)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

static SDRAM_HandleTypeDef sdramHandle;

/**
  * @brief  Programs the SDRAM device.
  * @param  RefreshCount: SDRAM refresh counter value
  * @retval None
  */
static void BSP_SDRAM_Initialization_sequence(uint32_t RefreshCount,
        uint32_t CasLatency)
{
    __IO uint32_t tmpmrd = 0;
    FMC_SDRAM_CommandTypeDef Command;

    /* Step 1: Configure a clock configuration enable command */
    Command.CommandMode            = FMC_SDRAM_CMD_CLK_ENABLE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = 0;

    /* Send the command */
    HAL_SDRAM_SendCommand(&sdramHandle, &Command, SDRAM_TIMEOUT);

    /* Step 2: Insert 100 us minimum delay */
    spin(1000);

    /* Step 3: Configure a PALL (precharge all) command */
    Command.CommandMode            = FMC_SDRAM_CMD_PALL;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = 0;

    /* Send the command */
    HAL_SDRAM_SendCommand(&sdramHandle, &Command, SDRAM_TIMEOUT);

    /* Step 4: Configure an Auto Refresh command */
    Command.CommandMode            = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 8;
    Command.ModeRegisterDefinition = 0;

    /* Send the command */
    HAL_SDRAM_SendCommand(&sdramHandle, &Command, SDRAM_TIMEOUT);

    /* Step 5: Program the external memory mode register */
    tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1          |\
             SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |\
             SDRAM_MODEREG_OPERATING_MODE_STANDARD |\
             SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    tmpmrd |= CasLatency;

    Command.CommandMode            = FMC_SDRAM_CMD_LOAD_MODE;
    Command.CommandTarget          = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber      = 1;
    Command.ModeRegisterDefinition = tmpmrd;

    /* Send the command */
    HAL_SDRAM_SendCommand(&sdramHandle, &Command, SDRAM_TIMEOUT);

    /* Step 6: Set the refresh rate counter */
    /* Set the device refresh rate */
    HAL_SDRAM_ProgramRefreshRate(&sdramHandle, RefreshCount);
}

static uint32_t GetMemoryWidth(sdram_config_t *config)
{
    switch (config->bus_width) {
        case SDRAM_BUS_WIDTH_8 :
            return FMC_SDRAM_MEM_BUS_WIDTH_8;
        case SDRAM_BUS_WIDTH_16 :
            return FMC_SDRAM_MEM_BUS_WIDTH_16;
        case SDRAM_BUS_WIDTH_32 :
            return FMC_SDRAM_MEM_BUS_WIDTH_32;
    }
    return 0;
}

static uint32_t GetColumnBitsNumber(sdram_config_t *config)
{
    switch (config->col_bits_num) {
        case SDRAM_COLUMN_BITS_8 :
            return FMC_SDRAM_COLUMN_BITS_NUM_8;
        case SDRAM_COLUMN_BITS_9 :
            return FMC_SDRAM_COLUMN_BITS_NUM_9;
        case SDRAM_COLUMN_BITS_10 :
            return FMC_SDRAM_COLUMN_BITS_NUM_10;
        case SDRAM_COLUMN_BITS_11 :
            return FMC_SDRAM_COLUMN_BITS_NUM_11;
    }
    return 0;
}

static uint32_t GetCasLatencyFMC(sdram_config_t *config)
{
    switch (config->cas_latency) {
        case SDRAM_CAS_LATENCY_1 :
            return FMC_SDRAM_CAS_LATENCY_1;
        case SDRAM_CAS_LATENCY_2 :
            return FMC_SDRAM_CAS_LATENCY_2;
        case SDRAM_CAS_LATENCY_3 :
            return FMC_SDRAM_CAS_LATENCY_3;
    }
    return 0;
}

static uint32_t GetCasLatencyModeReg(sdram_config_t *config)
{
    switch (config->cas_latency) {
        case SDRAM_CAS_LATENCY_1 :
            return SDRAM_MODEREG_CAS_LATENCY_1;
        case SDRAM_CAS_LATENCY_2 :
            return SDRAM_MODEREG_CAS_LATENCY_2;
        case SDRAM_CAS_LATENCY_3 :
            return SDRAM_MODEREG_CAS_LATENCY_3;
    }
    return 0;
}

/**
  * @brief  Initializes the SDRAM device.
  * @retval SDRAM status
  */
uint8_t stm32_sdram_init(sdram_config_t *config)
{
    static uint8_t sdramstatus = SDRAM_ERROR;
    static DMA_HandleTypeDef dma_handle;

    /* SDRAM device configuration */
    sdramHandle.Instance = FMC_SDRAM_DEVICE;

    /* Timing configuration for 100Mhz as SDRAM clock frequency (System clock is up to 200Mhz) */
    FMC_SDRAM_TimingTypeDef Timing;
    Timing.LoadToActiveDelay    = 2;
    Timing.ExitSelfRefreshDelay = 7;
    Timing.SelfRefreshTime      = 4;
    Timing.RowCycleDelay        = 7;
    Timing.WriteRecoveryTime    = 2;
    Timing.RPDelay              = 2;
    Timing.RCDDelay             = 2;

    sdramHandle.Init.SDBank             = FMC_SDRAM_BANK1;
    sdramHandle.Init.ColumnBitsNumber   = GetColumnBitsNumber(config);
    sdramHandle.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
    sdramHandle.Init.MemoryDataWidth    = GetMemoryWidth(config);
    sdramHandle.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    sdramHandle.Init.CASLatency         = GetCasLatencyFMC(config);
    sdramHandle.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    sdramHandle.Init.SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2;
    sdramHandle.Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE;
    sdramHandle.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0;

    /* Enable FMC clock */
    __HAL_RCC_FMC_CLK_ENABLE();

    /* Enable chosen DMAx clock */
    __DMAx_CLK_ENABLE();

    /* SDRAM GPIO initialization */
    stm_sdram_GPIO_init();

    /* Configure common DMA parameters */
    dma_handle.Init.Channel             = SDRAM_DMAx_CHANNEL;
    dma_handle.Init.Direction           = DMA_MEMORY_TO_MEMORY;
    dma_handle.Init.PeriphInc           = DMA_PINC_ENABLE;
    dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
    dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    dma_handle.Init.Mode                = DMA_NORMAL;
    dma_handle.Init.Priority            = DMA_PRIORITY_HIGH;
    dma_handle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    dma_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    dma_handle.Init.MemBurst            = DMA_MBURST_SINGLE;
    dma_handle.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    dma_handle.Instance = SDRAM_DMAx_STREAM;

    /* Associate the DMA handle */
    __HAL_LINKDMA(&sdramHandle, hdma, dma_handle);

    /* Deinitialize the stream for new transfer */
    HAL_DMA_DeInit(&dma_handle);

    /* Configure the DMA stream */
    HAL_DMA_Init(&dma_handle);

#if 0
    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority(SDRAM_DMAx_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDRAM_DMAx_IRQn);
#endif

    if (HAL_SDRAM_Init(&sdramHandle, &Timing) != HAL_OK) {
        sdramstatus = SDRAM_ERROR;
    } else {
        sdramstatus = SDRAM_OK;
    }

    /* SDRAM initialization sequence */
    BSP_SDRAM_Initialization_sequence(REFRESH_COUNT,
                                      GetCasLatencyModeReg(config));

    return sdramstatus;
}


