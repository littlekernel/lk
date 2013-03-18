#ifndef __PLATFORM_STELLARIS_GPIO_H
#define __PLATFORM_STELLARIS_GPIO_H

/* helper defines for Stellaris platforms */

/* flag to gpio_configure */
#define GPIO_STELLARIS_OD (0x1 << 12)
#define GPIO_STELLARIS_AF_ENABLE (0x2 << 12)

#define GPIO_STELLARIS_AF(x) (((x) & 0xf) << 8)

/* gpio port/pin is packed into a single unsigned int in 20x:4alternatefunc:4port:4pin format */
#define GPIO(port, pin) ((unsigned int)(((port) << 4) | (pin)))

#define GPIO_PORT(gpio) (((gpio) >> 4) & 0xf)
#define GPIO_PIN(gpio) ((gpio) & 0xf)

#define GPIO_PORT_A 0
#define GPIO_PORT_B 1
#define GPIO_PORT_C 2
#define GPIO_PORT_D 3
#define GPIO_PORT_E 4
#define GPIO_PORT_F 5
#define GPIO_PORT_G 6
#define GPIO_PORT_H 7
/* discontinuity */
#define GPIO_PORT_J 8
#define GPIO_PORT_K 9
#define GPIO_PORT_L 10
#define GPIO_PORT_M 11
#define GPIO_PORT_N 12
/* discontinuity */
#define GPIO_PORT_P 13
#define GPIO_PORT_Q 14

#endif

