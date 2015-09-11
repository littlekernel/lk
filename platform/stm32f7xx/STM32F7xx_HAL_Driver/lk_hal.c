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
#include <debug.h>
#include <platform.h>
#include <platform/stm32.h>

/* overrides for certain hal routines, implemented in terms of LK's api  */

/**
  * @brief Provides a tick value in millisecond.
  * @retval tick value
  */
uint32_t HAL_GetTick(void)
{
    return current_time();
}

/**
  * @brief This function provides accurate delay (in milliseconds) based
  *        on variable incremented.
  * @param Delay: specifies the delay time length, in milliseconds.
  * @retval None
  */
void HAL_Delay(__IO uint32_t Delay)
{
    spin(Delay * 1000);
}

/**
  * @brief Suspend Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once HAL_SuspendTick()
  *       is called, the the SysTick interrupt will be disabled and so Tick increment
  *       is suspended.
  * @retval None
  */
void HAL_SuspendTick(void)
{
    /* we don't want any one calling this */
    PANIC_UNIMPLEMENTED;
}

/**
  * @brief Resume Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once HAL_ResumeTick()
  *       is called, the the SysTick interrupt will be enabled and so Tick increment
  *       is resumed.
  * @retval None
  */
void HAL_ResumeTick(void)
{
    /* we don't want any one calling this */
    PANIC_UNIMPLEMENTED;
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    /* Start the systick at 10ms. The kernel will later override this */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/10000);

    return HAL_OK;
}

