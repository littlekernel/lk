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
#include <err.h>
#include <debug.h>
#include <stdlib.h>
#include <reg.h>
#include <dev/uart.h>
#include <platform.h>
#include <platform/stm32.h>
#include <platform/sdram.h>
#include <kernel/novm.h>
#include <arch/arm/cm.h>

uint32_t SystemCoreClock = HSI_VALUE;
uint32_t stm32_unique_id[3];

#if defined(ENABLE_SDRAM)
// target exports this with sdram configuration values
extern const sdram_config_t target_sdram_config;
#endif

void SystemInit(void)
{
    /* Reset the RCC clock configuration to the default reset state ------------*/
    /* Set HSION bit */
    RCC->CR |= (uint32_t)0x00000001;

    /* Reset CFGR register */
    RCC->CFGR = 0x00000000;

    /* Reset HSEON, CSSON and PLLON bits */
    RCC->CR &= (uint32_t)0xFEF6FFFF;

    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x24003010;

    /* Reset HSEBYP bit */
    RCC->CR &= (uint32_t)0xFFFBFFFF;

    /* Disable all interrupts */
    RCC->CIR = 0x00000000;

    __HAL_RCC_PWR_CLK_ENABLE();

#if defined (DATA_IN_ExtSRAM) || defined (DATA_IN_ExtSDRAM)
    SystemInit_ExtMemCtl();
#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE or HSI)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25 or 16
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    HAL_StatusTypeDef ret = HAL_OK;

#if defined(USE_HSE_XTAL)
    /* Enable HSE Oscillator and activate PLL with HSE as source.
     * The external XTAL is a more stable clock source.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 432;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 9;
#else
    /* Enable HSI Oscillator and activate PLL with HSI as source.
     * Some boards like STm32756G-EVAL2 seem to malfuction with the
     * HSE xtal configuration.
     */
    RCC_OscInitStruct.OscillatorType    = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState          = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue  = 16;
    RCC_OscInitStruct.PLL.PLLState      = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource     = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM          = 16;
    RCC_OscInitStruct.PLL.PLLN          = 432;
    RCC_OscInitStruct.PLL.PLLP          = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ          = 9;
#endif

    ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
    if (ret != HAL_OK) {
        while (1) { ; }
    }

    /* Activate the OverDrive to reach the 216 MHz Frequency */
    ret = HAL_PWREx_EnableOverDrive();
    if (ret != HAL_OK) {
        while (1) { ; }
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
    if (ret != HAL_OK) {
        while (1) { ; }
    }
}

void stm32_rng_init(void)
{
    RNG_HandleTypeDef rng_handle = { 0 };

    __HAL_RCC_RNG_CLK_ENABLE();

    rng_handle.Instance = RNG;
    HAL_StatusTypeDef status = HAL_RNG_Init(&rng_handle);
    if (status != HAL_OK) {
        panic("error initializing random number hardware\n");
    }

    /* seed the pseudo random number generator with this */
#if STM32_SEED_RAND_FROM_HWRNG
    uint32_t r;

    /* discard he first result */
    status = HAL_RNG_GenerateRandomNumber(&rng_handle, &r);
    if (status != HAL_OK) {
        panic("error getting random number from hardware\n");
    }

    status = HAL_RNG_GenerateRandomNumber(&rng_handle, &r);
    if (status != HAL_OK) {
        panic("error getting random number from hardware\n");
    }

    srand(r);
#endif
}

/* set up the mpu to enable caching in the appropriate areas */
static void mpu_init(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct;
    HAL_MPU_Disable();

    uint region_num = 0;

    /* mark the first bit of the address space as inaccessible, to catch null pointers */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x0;
    MPU_InitStruct.Size = MPU_REGION_SIZE_128KB; /* 0x00200000 */
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = region_num++;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

#if defined(ENABLE_SDRAM)
    /* configure SDRAM */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = SDRAM_BASE;

    MPU_InitStruct.Size =
#if   SDRAM_SIZE == 0x00100000
        MPU_REGION_SIZE_1MB;
#elif SDRAM_SIZE == 0x00200000
        MPU_REGION_SIZE_2MB;
#elif SDRAM_SIZE == 0x00400000
        MPU_REGION_SIZE_4MB;
#elif SDRAM_SIZE == 0x00800000
        MPU_REGION_SIZE_8MB;
#elif SDRAM_SIZE == 0x01000000
        MPU_REGION_SIZE_16MB
#elif SDRAM_SIZE == 0x02000000
        MPU_REGION_SIZE_32MB;
#elif SDRAM_SIZE == 0x04000000
        MPU_REGION_SIZE_64MB;
#elif SDRAM_SIZE == 0x08000000
        MPU_REGION_SIZE_128MB;
#else
#error SDRAM_SIZE not defined.
#endif

    MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RW;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = region_num++;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

    // SRAM
#if defined(ENABLE_EXT_SRAM)
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = EXT_SRAM_BASE;
    MPU_InitStruct.Size = MPU_REGION_SIZE_2MB; // XXX use max size of aperture?
    MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RW;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = region_num++;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

    HAL_MPU_Enable(MPU_HFNMI_PRIVDEF);
}

void platform_early_init(void)
{
    // Do general system init
    SystemInit();
    SystemClock_Config();

    // Enable the flash ART controller
    __HAL_FLASH_ART_ENABLE();
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();

    /* read the unique id */
    stm32_unique_id[0] = *REG32(0x1ff0f420);
    stm32_unique_id[1] = *REG32(0x1ff0f424);
    stm32_unique_id[2] = *REG32(0x1ff0f428);

    /* seed the random number generator based on this */
    srand(stm32_unique_id[0] ^ stm32_unique_id[1] ^ stm32_unique_id[2]);

    // Start the systick timer
    uint32_t sysclk = HAL_RCC_GetSysClockFreq();
    arm_cm_systick_init(sysclk);

    stm32_timer_early_init();
    stm32_gpio_early_init();
    stm32_flash_early_init();
    stm32_rng_init();
    stm32_usbc_early_init();

    /* clear the reboot reason */
    RCC->CSR |= (1<<24);

#if defined(ENABLE_SDRAM)
    /* initialize SDRAM */
    stm32_sdram_init((sdram_config_t *)&target_sdram_config);

    /* add a novm arena for it */
    novm_add_arena("sdram", SDRAM_BASE, SDRAM_SIZE);
#endif

    mpu_init();
}

void platform_init(void)
{
    printf("clocks:\n");
    printf("\tsysclk %u\n", HAL_RCC_GetSysClockFreq());
    printf("\thclk %u\n", HAL_RCC_GetHCLKFreq());
    printf("\tpclk1 %u\n", HAL_RCC_GetPCLK1Freq());
    printf("\tpclk2 %u\n", HAL_RCC_GetPCLK2Freq());

    printf("unique id: 0x%08x%08x%08x\n", stm32_unique_id[0], stm32_unique_id[1], stm32_unique_id[2]);

    stm32_timer_init();

    stm32_flash_init();

    stm32_usbc_init();
}

