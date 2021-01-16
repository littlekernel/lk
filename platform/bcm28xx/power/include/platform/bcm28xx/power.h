#pragma once

#include <platform/bcm28xx/pm.h>
#include <stdint.h>

#define BIT(n) (1<<n)

#define PM_V3DRSTN  BIT(6)
#define PM_ISFUNC   BIT(5)
#define PM_MRDONE   BIT(4)
#define PM_MEMREP   BIT(3)
#define PM_ISPOW    BIT(2)
#define PM_POWOK    BIT(1)
#define PM_POWUP    BIT(0)

void power_up_image(void);
void power_up_usb(void);
void power_domain_on(volatile uint32_t *reg);
