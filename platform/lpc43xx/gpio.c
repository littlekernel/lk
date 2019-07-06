/*
 * Copyright (c) 2015 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/reg.h>
#include <platform/lpc43xx-gpio.h>

inline int gpio_config(unsigned nr, unsigned flags) {
    unsigned m = _GPIOm(nr);
    unsigned n = _GPIOn(nr);
    if (flags & GPIO_INPUT) {
        writel(readl(GPIO_DIR(m)) & (~(1 << n)), GPIO_DIR(m));
    } else {
        writel(readl(GPIO_DIR(m)) | (1 << n), GPIO_DIR(m));
    }
    return 0;
}

inline void gpio_set(unsigned nr, unsigned on) {
    writel(on, GPIO_WORD(nr));
}

inline int gpio_get(unsigned nr) {
    return readl(GPIO_WORD(nr)) & 1;
}
