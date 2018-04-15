#ifndef __PLATFORM_STM32_EXTI_H
#define __PLATFORM_STM32_EXTI_H

#include <stdbool.h>
#include <stm32f0xx.h>

typedef enum {
#ifdef SYSCFG_EXTICR1_EXTI0_PA
    EXT_INTERRUPT_PORT_A = SYSCFG_EXTICR1_EXTI0_PA,
#endif
#ifdef SYSCFG_EXTICR1_EXTI0_PB
    EXT_INTERRUPT_PORT_B = SYSCFG_EXTICR1_EXTI0_PB,
#endif
#ifdef SYSCFG_EXTICR1_EXTI0_PC
    EXT_INTERRUPT_PORT_C = SYSCFG_EXTICR1_EXTI0_PC,
#endif
#ifdef SYSCFG_EXTICR1_EXTI0_PD
    EXT_INTERRUPT_PORT_D = SYSCFG_EXTICR1_EXTI0_PD,
#endif
#ifdef SYSCFG_EXTICR1_EXTI0_PE
    EXT_INTERRUPT_PORT_E = SYSCFG_EXTICR1_EXTI0_PE,
#endif
#ifdef SYSCFG_EXTICR1_EXTI0_PF
    EXT_INTERRUPT_PORT_F = SYSCFG_EXTICR1_EXTI0_PF,
#endif
} stm32_ext_interrupt_port_t;

void stm32_setup_ext_interrupt(int interrupt, stm32_ext_interrupt_port_t port,
                               bool rising_edge, bool falling_edge);

// Define these in your target code to override the weak, no-op default
// handlers.  These should return true if lk should reschedule after handling.
bool stm32_exti0_irq(void);
bool stm32_exti1_irq(void);
bool stm32_exti2_irq(void);
bool stm32_exti3_irq(void);
bool stm32_exti4_irq(void);
bool stm32_exti5_irq(void);
bool stm32_exti6_irq(void);
bool stm32_exti7_irq(void);
bool stm32_exti8_irq(void);
bool stm32_exti9_irq(void);
bool stm32_exti10_irq(void);
bool stm32_exti11_irq(void);
bool stm32_exti12_irq(void);
bool stm32_exti13_irq(void);
bool stm32_exti14_irq(void);
bool stm32_exti15_irq(void);

#endif  // __PLATFORM_STM32_EXTI_H
