/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <assert.h>
#include <lk/debug.h>
#include <dev/gpio.h>
#include <platform/gpio.h>
#include <platform/rcc.h>
#include <platform/stm32.h>
#include <stm32h5xx.h>

typedef GPIO_TypeDef stm32_gpio_t;

typedef enum {
    STM32_GPIO_SPEED_LOW       = 0x0,
    STM32_GPIO_SPEED_MEDIUM    = 0x1,
    STM32_GPIO_SPEED_HIGH      = 0x2,
    STM32_GPIO_SPEED_VERY_HIGH = 0x3,
} stm32_gpio_speed_t;

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

struct gpio_port {
    stm32_gpio_t *regs;
    stm32_rcc_clk_t clock;
};

static const struct gpio_port gpio_ports[] = {
    [GPIO_PORT_A] = { GPIOA, STM32_RCC_CLK_GPIOA },
    [GPIO_PORT_B] = { GPIOB, STM32_RCC_CLK_GPIOB },
    [GPIO_PORT_C] = { GPIOC, STM32_RCC_CLK_GPIOC },
    [GPIO_PORT_D] = { GPIOD, STM32_RCC_CLK_GPIOD },
    [GPIO_PORT_E] = { GPIOE, STM32_RCC_CLK_GPIOE },
    [GPIO_PORT_F] = { GPIOF, STM32_RCC_CLK_GPIOF },
    [GPIO_PORT_G] = { GPIOG, STM32_RCC_CLK_GPIOG },
    [GPIO_PORT_H] = { GPIOH, STM32_RCC_CLK_GPIOH },
    [GPIO_PORT_I] = { GPIOI, STM32_RCC_CLK_GPIOI },
};

static stm32_gpio_t *stm32_gpio_port_to_pointer(unsigned int port) {
    DEBUG_ASSERT(port < countof(gpio_ports));

    return gpio_ports[port].regs;
}

/*
 * Enable every port's clock up front. There are nine of them and the H563 has
 * plenty of flash, so this keeps gpio_config() free of ordering concerns.
 */
void stm32_gpio_early_init(void) {
    for (size_t i = 0; i < countof(gpio_ports); i++) {
        stm32_rcc_set_enable(gpio_ports[i].clock, true);
    }
}

static void stm32_gpio_af_config(stm32_gpio_t *gpio, uint32_t pin,
                                 uint32_t af_num) {
    // 8 AF entries per register
    uint32_t reg_index = pin >> 3;
    uint32_t entry_shift = (pin & 0x7) * 4;

    gpio->AFR[reg_index] &= ~(0xf << entry_shift);
    gpio->AFR[reg_index] |= (af_num & 0xf) << entry_shift;
}

int gpio_config(unsigned nr, unsigned flags) {
    uint32_t port = GPIO_PORT(nr);
    uint32_t pin = GPIO_PIN(nr);
    stm32_gpio_t *gpio = stm32_gpio_port_to_pointer(port);

    assert(pin < 16);

    if (flags & GPIO_STM32_AF) {
        stm32_gpio_af_config(gpio, pin, GPIO_AFNUM(flags));
    }

    if ((flags & GPIO_OUTPUT) || (flags & GPIO_STM32_AF)) {
        gpio->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED0 << (pin * 2));
        gpio->OSPEEDR |= STM32_GPIO_SPEED_VERY_HIGH << (pin * 2);

        // Output mode configuration
        gpio->OTYPER &= ~(GPIO_OTYPER_OT0 << pin);
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

    gpio->MODER &= ~(GPIO_MODER_MODE0 << (pin * 2));
    gpio->MODER |= (mode << (pin * 2));

    stm32_gpio_pupd_t pupd = STM32_GPIO_PUPD_NONE;
    if (flags & GPIO_PULLUP) {
        pupd = STM32_GPIO_PUPD_UP;
    } else if (flags & GPIO_PULLDOWN) {
        pupd = STM32_GPIO_PUPD_DOWN;
    }

    gpio->PUPDR &= ~(GPIO_PUPDR_PUPD0 << (pin * 2));
    gpio->PUPDR |= pupd << (pin * 2);

    return 0;
}

void gpio_set(unsigned nr, unsigned on) {
    stm32_gpio_t *gpio = stm32_gpio_port_to_pointer(GPIO_PORT(nr));
    if (on) {
        gpio->BSRR = 1 << GPIO_PIN(nr);
    } else {
        gpio->BRR = 1 << GPIO_PIN(nr);
    }
}

int gpio_get(unsigned nr) {
    stm32_gpio_t *gpio = stm32_gpio_port_to_pointer(GPIO_PORT(nr));
    return (gpio->IDR & (1 << GPIO_PIN(nr))) != 0;
}
