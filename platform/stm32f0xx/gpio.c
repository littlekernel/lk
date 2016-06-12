/*
 * Copyright (c) 2012 Travis Geiselbrecht
 * Copyright (c) 2016 Erik Gilling
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

#include <assert.h>
#include <debug.h>
#include <dev/gpio.h>
#include <platform/gpio.h>
#include <platform/rcc.h>
#include <platform/stm32.h>
#include <stm32f0xx.h>

typedef GPIO_TypeDef stm32_gpio_t;

typedef enum {
    STM32_GPIO_SPEED_2_MHZ  = 0x0,
    STM32_GPIO_SPEED_20_MHZ = 0x1,
    STM32_GPIO_SPEED_50_MHZ = 0x3,
} stm32_goio_speed_t;

typedef enum {
    STM32_GPIO_OTYPE_PP = 0x0,
    STM32_GPIO_OTYPE_OD = 0x1,
} stm32_gpio_otype_t;

typedef enum {
    STM32_GPIO_MODE_IN  = 0x0,
    STM32_GPIO_MODE_OUT = 0x1,
    STM32_GPIO_MODE_AF  = 0x2,
    STM32_GPIO_MODE_AN  = 0x3,
} stm32_gpio_mode_t;

typedef enum {
    STM32_GPIO_PUPD_NONE = 0x0,
    STM32_GPIO_PUPD_UP   = 0x1,
    STM32_GPIO_PUPD_DOWN = 0x2,
} stm32_gpio_pupd_t;

static stm32_gpio_t *stm32_gpio_port_to_pointer(unsigned int port)
{
    switch (port) {
        default:
        case GPIO_PORT_A:
            return GPIOA;
        case GPIO_PORT_B:
            return GPIOB;
        case GPIO_PORT_C:
            return GPIOC;
        case GPIO_PORT_D:
            return GPIOD;
        case GPIO_PORT_E:
            return GPIOE;
        case GPIO_PORT_F:
            return GPIOF;
    }
}

static void stm32_gpio_enable_port(unsigned int port)
{
    DEBUG_ASSERT(port <= GPIO_PORT_F);

    switch (port) {
        default:
        case GPIO_PORT_A:
            stm32_rcc_set_enable(STM32_RCC_CLK_IOPA, true);
            break;

        case GPIO_PORT_B:
            stm32_rcc_set_enable(STM32_RCC_CLK_IOPB, true);
            break;

        case GPIO_PORT_C:
            stm32_rcc_set_enable(STM32_RCC_CLK_IOPC, true);
            break;

        case GPIO_PORT_D:
            stm32_rcc_set_enable(STM32_RCC_CLK_IOPD, true);
            break;

        case GPIO_PORT_E:
            stm32_rcc_set_enable(STM32_RCC_CLK_IOPE, true);
            break;

        case GPIO_PORT_F:
            stm32_rcc_set_enable(STM32_RCC_CLK_IOPF, true);
            break;

    }
}

void stm32_gpio_early_init(void)
{
}

static void stm32_gpio_af_config(stm32_gpio_t *gpio, uint32_t pin,
                                 uint32_t af_num) {
  // 8 AF entries per register
  uint32_t reg_index = pin >> 3;
  uint32_t entry_shift = (pin & 0x7) * 4;

  gpio->AFR[reg_index] &= ~(0xf << entry_shift);
  gpio->AFR[reg_index] |= (af_num & 0xf) << entry_shift;
}

int gpio_config(unsigned nr, unsigned flags)
{
    uint32_t port = GPIO_PORT(nr);
    uint32_t pin = GPIO_PIN(nr);
    stm32_gpio_t *gpio = stm32_gpio_port_to_pointer(port);

    assert(pin < 16);

    stm32_gpio_enable_port(port);

    if (flags & GPIO_STM32_AF) {
        stm32_gpio_af_config(gpio, pin, GPIO_AFNUM(flags));
    }

    if ((flags & GPIO_OUTPUT) || (flags & GPIO_STM32_AF)) {
        // All pins configured to 50MHz.
        gpio->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR0 << (pin * 2));
        gpio->OSPEEDR |= STM32_GPIO_SPEED_50_MHZ << (pin * 2);

        // Output mode configuration
        gpio->OTYPER &= ~((GPIO_OTYPER_OT_0) << pin);
        if (flags & GPIO_STM32_OD) {
            gpio->OTYPER |= STM32_GPIO_OTYPE_OD << pin;
        } else {
            gpio->OTYPER |= STM32_GPIO_OTYPE_PP << pin;
        }
    }

    stm32_gpio_mode_t mode;
    if (flags & GPIO_OUTPUT) {
        mode = STM32_GPIO_MODE_OUT;
    } else if (flags & GPIO_STM32_AF) {
        mode = STM32_GPIO_MODE_AF;
    } else {
        mode = STM32_GPIO_MODE_IN;
    }

    gpio->MODER  &= ~(GPIO_MODER_MODER0 << (pin * 2));
    gpio->MODER |= (mode << (pin * 2));

    stm32_gpio_pupd_t pupd = STM32_GPIO_PUPD_NONE;
    if (flags & GPIO_PULLUP) {
        pupd = STM32_GPIO_PUPD_UP;
    } else if (flags & GPIO_PULLDOWN) {
        pupd = STM32_GPIO_PUPD_DOWN;
    }

    gpio->PUPDR &= ~(GPIO_PUPDR_PUPDR0 << (pin * 2));
    gpio->PUPDR |= pupd << (pin * 2);

    return 0;
}

void gpio_set(unsigned nr, unsigned on)
{
    stm32_gpio_t *gpio = stm32_gpio_port_to_pointer(nr);
    if (on) {
        gpio->BSRR = 1 << GPIO_PIN(nr);
    } else {
        gpio->BRR = 1 << GPIO_PIN(nr);
    }
}

int gpio_get(unsigned nr)
{
    stm32_gpio_t *gpio = stm32_gpio_port_to_pointer(GPIO_PORT(nr));
    return (gpio->IDR & (1 << GPIO_PIN(nr))) != 0;
}

