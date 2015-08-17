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
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <dev/gpio.h>
#include <platform/stm32.h>
#include <platform/gpio.h>
#include <target/debugconfig.h>
#include <target/gpioconfig.h>
#include <reg.h>

extern uint8_t BSP_SDRAM_Init(void);

void target_early_init(void)
{
#if DEBUG_UART == 1
    /* configure usart 1 pins */
    gpio_config(GPIO_USART1_TX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF7_USART1) | GPIO_PULLUP);
    gpio_config(GPIO_USART1_RX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF7_USART1) | GPIO_PULLUP);
#else
#error need to configure gpio pins for debug uart
#endif

    /* now that the uart gpios are configured, enable the debug uart */
    stm32_debug_early_init();

    /* initialize sdram */
    BSP_SDRAM_Init();
}

void target_init(void)
{
    stm32_debug_init();
}

#if 0
void target_set_debug_led(unsigned int led, bool on)
{
    switch (led) {
    case 0:
        gpio_set(GPIO_LED0, on);
        break;
    case 1:
        gpio_set(GPIO_LED1, on);
        break;
    case 2:
        gpio_set(GPIO_LED2, on);
        break;
    case 3:
        gpio_set(GPIO_LED3, on);
        break;
    }
}
#endif
