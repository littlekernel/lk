/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <hbird.h>

#include <lk/compiler.h>
#include <lk/debug.h>

/* un-overridden irq handler */
void hbird_dummy_irq(void) {
    panic("unhandled irq\n");
}

/* a list of default handlers that are simply aliases to the dummy handler */
#define DEFAULT_HANDLER(x) \
void hbird_irq##x##_handler(void) __WEAK_ALIAS("hbird_dummy_irq");

DEFAULT_HANDLER(19);
DEFAULT_HANDLER(20);
DEFAULT_HANDLER(21);
DEFAULT_HANDLER(22);
DEFAULT_HANDLER(23);
DEFAULT_HANDLER(24);
DEFAULT_HANDLER(25);
DEFAULT_HANDLER(26);
DEFAULT_HANDLER(27);
DEFAULT_HANDLER(28);
DEFAULT_HANDLER(29);
DEFAULT_HANDLER(30);
DEFAULT_HANDLER(31);
DEFAULT_HANDLER(32);
DEFAULT_HANDLER(33);
DEFAULT_HANDLER(34);
DEFAULT_HANDLER(35);
DEFAULT_HANDLER(36);
DEFAULT_HANDLER(37);
DEFAULT_HANDLER(38);
DEFAULT_HANDLER(39);
DEFAULT_HANDLER(40);
DEFAULT_HANDLER(41);
DEFAULT_HANDLER(42);
DEFAULT_HANDLER(43);
DEFAULT_HANDLER(44);
DEFAULT_HANDLER(45);
DEFAULT_HANDLER(46);
DEFAULT_HANDLER(47);
DEFAULT_HANDLER(48);
DEFAULT_HANDLER(49);
DEFAULT_HANDLER(50);

#define VECTAB_ENTRY(x) [SOC_INT##x##_IRQn] = hbird_irq##x##_handler

/*
 * Appended to the end of the nuclei riscv vector table.
 *
 */
const void *const __SECTION(".text.boot.vectab2") vectab2[] = {
    VECTAB_ENTRY(19),
    VECTAB_ENTRY(20),
    VECTAB_ENTRY(21),
    VECTAB_ENTRY(22),
    VECTAB_ENTRY(23),
    VECTAB_ENTRY(24),
    VECTAB_ENTRY(25),
    VECTAB_ENTRY(26),
    VECTAB_ENTRY(27),
    VECTAB_ENTRY(28),
    VECTAB_ENTRY(29),
    VECTAB_ENTRY(30),
    VECTAB_ENTRY(31),
    VECTAB_ENTRY(32),
    VECTAB_ENTRY(33),
    VECTAB_ENTRY(34),
    VECTAB_ENTRY(35),
    VECTAB_ENTRY(36),
    VECTAB_ENTRY(37),
    VECTAB_ENTRY(38),
    VECTAB_ENTRY(39),
    VECTAB_ENTRY(40),
    VECTAB_ENTRY(41),
    VECTAB_ENTRY(42),
    VECTAB_ENTRY(43),
    VECTAB_ENTRY(44),
    VECTAB_ENTRY(45),
    VECTAB_ENTRY(46),
    VECTAB_ENTRY(47),
    VECTAB_ENTRY(48),
    VECTAB_ENTRY(49),
    VECTAB_ENTRY(50),
};
