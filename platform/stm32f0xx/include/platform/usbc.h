#ifndef __PLATFORM_STM32_USB_H
#define __PLATFORM_STM32_USB_H

typedef enum {
    STM32_USB_CLK_PLL = 0,
    STM32_USB_CLK_HSI48 = 1,
} stm32_usb_clk_t;

void stm32_usbc_early_init(stm32_usb_clk_t clock_source);
void stm32_usbc_init(void);

#endif // __PLATFORM_STM32_USB_H
