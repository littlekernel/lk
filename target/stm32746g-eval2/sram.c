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

#define   SRAM_OK         ((uint8_t)0x00)
#define   SRAM_ERROR      ((uint8_t)0x01)

/* #define SRAM_MEMORY_WIDTH    FMC_NORSRAM_MEM_BUS_WIDTH_8*/
#define SRAM_MEMORY_WIDTH    FMC_NORSRAM_MEM_BUS_WIDTH_16

#define SRAM_BURSTACCESS     FMC_BURST_ACCESS_MODE_DISABLE
//#define SRAM_BURSTACCESS     FMC_BURST_ACCESS_MODE_ENABLE

#define SRAM_WRITEBURST      FMC_WRITE_BURST_DISABLE
//#define SRAM_WRITEBURST     FMC_WRITE_BURST_ENABLE

#define CONTINUOUSCLOCK_FEATURE    FMC_CONTINUOUS_CLOCK_SYNC_ONLY
//#define CONTINUOUSCLOCK_FEATURE     FMC_CONTINUOUS_CLOCK_SYNC_ASYNC

/* DMA definitions for SRAM DMA transfer */
#define __SRAM_DMAx_CLK_ENABLE            __HAL_RCC_DMA2_CLK_ENABLE
#define __SRAM_DMAx_CLK_DISABLE           __HAL_RCC_DMA2_CLK_DISABLE
#define SRAM_DMAx_CHANNEL                 DMA_CHANNEL_0
#define SRAM_DMAx_STREAM                  DMA2_Stream4
#define SRAM_DMAx_IRQn                    DMA2_Stream4_IRQn
#define SRAM_DMAx_IRQHandler              DMA2_Stream4_IRQHandler

static SRAM_HandleTypeDef sramHandle;
static FMC_NORSRAM_TimingTypeDef Timing;

/**
  * @brief  Initializes SRAM MSP.
  * @param  hsram: SRAM handle
  * @retval None
  */
static void BSP_SRAM_MspInit(SRAM_HandleTypeDef  *hsram, void *Params)
{
    static DMA_HandleTypeDef dma_handle;
    GPIO_InitTypeDef gpio_init_structure;

    /* Enable FMC clock */
    __HAL_RCC_FMC_CLK_ENABLE();

    /* Enable chosen DMAx clock */
    __SRAM_DMAx_CLK_ENABLE();

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* Common GPIO configuration */
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_PULLUP;
    gpio_init_structure.Speed     = GPIO_SPEED_HIGH;
    gpio_init_structure.Alternate = GPIO_AF12_FMC;

    /* GPIOD configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8     |\
                                GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |\
                                GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio_init_structure);

    /* GPIOE configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3| GPIO_PIN_4 | GPIO_PIN_7     |\
                                GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |\
                                GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio_init_structure);

    /* GPIOF configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4     |\
                                GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &gpio_init_structure);

    /* GPIOG configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4     |\
                                GPIO_PIN_5 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOG, &gpio_init_structure);

    /* Configure common DMA parameters */
    dma_handle.Init.Channel             = SRAM_DMAx_CHANNEL;
    dma_handle.Init.Direction           = DMA_MEMORY_TO_MEMORY;
    dma_handle.Init.PeriphInc           = DMA_PINC_ENABLE;
    dma_handle.Init.MemInc              = DMA_MINC_ENABLE;
    dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    dma_handle.Init.Mode                = DMA_NORMAL;
    dma_handle.Init.Priority            = DMA_PRIORITY_HIGH;
    dma_handle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    dma_handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    dma_handle.Init.MemBurst            = DMA_MBURST_SINGLE;
    dma_handle.Init.PeriphBurst         = DMA_PBURST_SINGLE;

    dma_handle.Instance = SRAM_DMAx_STREAM;

    /* Associate the DMA handle */
    __HAL_LINKDMA(hsram, hdma, dma_handle);

    /* Deinitialize the Stream for new transfer */
    HAL_DMA_DeInit(&dma_handle);

    /* Configure the DMA Stream */
    HAL_DMA_Init(&dma_handle);

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority(SRAM_DMAx_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SRAM_DMAx_IRQn);
}

/**
  * @brief  Initializes the SRAM device.
  * @retval SRAM status
  */
uint8_t BSP_SRAM_Init(void)
{
    static uint8_t sram_status = SRAM_ERROR;
    /* SRAM device configuration */
    sramHandle.Instance = FMC_NORSRAM_DEVICE;
    sramHandle.Extended = FMC_NORSRAM_EXTENDED_DEVICE;

    /* SRAM device configuration */
    /* Timing configuration derived from system clock (up to 216Mhz)
       for 108Mhz as SRAM clock frequency */
    Timing.AddressSetupTime      = 2;
    Timing.AddressHoldTime       = 1;
    Timing.DataSetupTime         = 2;
    Timing.BusTurnAroundDuration = 1;
    Timing.CLKDivision           = 2;
    Timing.DataLatency           = 2;
    Timing.AccessMode            = FMC_ACCESS_MODE_A;

    sramHandle.Init.NSBank             = FMC_NORSRAM_BANK3;
    sramHandle.Init.DataAddressMux     = FMC_DATA_ADDRESS_MUX_DISABLE;
    sramHandle.Init.MemoryType         = FMC_MEMORY_TYPE_SRAM;
    sramHandle.Init.MemoryDataWidth    = SRAM_MEMORY_WIDTH;
    sramHandle.Init.BurstAccessMode    = SRAM_BURSTACCESS;
    sramHandle.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
    sramHandle.Init.WaitSignalActive   = FMC_WAIT_TIMING_BEFORE_WS;
    sramHandle.Init.WriteOperation     = FMC_WRITE_OPERATION_ENABLE;
    sramHandle.Init.WaitSignal         = FMC_WAIT_SIGNAL_DISABLE;
    sramHandle.Init.ExtendedMode       = FMC_EXTENDED_MODE_DISABLE;
    sramHandle.Init.AsynchronousWait   = FMC_ASYNCHRONOUS_WAIT_DISABLE;
    sramHandle.Init.WriteBurst         = SRAM_WRITEBURST;
    sramHandle.Init.ContinuousClock    = CONTINUOUSCLOCK_FEATURE;

    /* SRAM controller initialization */
    BSP_SRAM_MspInit(&sramHandle, NULL); /* __weak function can be rewritten by the application */
    if (HAL_SRAM_Init(&sramHandle, &Timing, &Timing) != HAL_OK) {
        sram_status = SRAM_ERROR;
    } else {
        sram_status = SRAM_OK;
    }
    return sram_status;
}




