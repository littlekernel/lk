/*
 * Copyright (c) 2015 Christopher Anderson
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

#include <dev/gpio.h>
#include <lib/console.h>
#include <string.h>
#include <stdio.h>

#if WITH_LIB_CONSOLE

struct flag_labels {
    unsigned id;
    const char *label;
};

struct flag_labels gpio_flag_labels[] = {
    { GPIO_INPUT,       "input" },
    { GPIO_OUTPUT,      "output" },
    { GPIO_LEVEL,       "level" },
    { GPIO_EDGE,        "edge" },
    { GPIO_RISING,      "rising" },
    { GPIO_FALLING,     "falling" },
    { GPIO_HIGH,        "high" },
    { GPIO_LOW,         "low" },
    { GPIO_PULLUP,      "pullup" },
    { GPIO_PULLDOWN,    "pulldown" },
};

static unsigned int get_flag_value(const char *str)
{
    for (unsigned i = 0; i < countof(gpio_flag_labels); i++) {
        if (!strcmp(str, gpio_flag_labels[i].label)) {
            return gpio_flag_labels[i].id;
        }
    }

    return 0;
}

/* Ideally this would be generic, but different platforms have varying manners of handling gpio
 * numbers / banks etc. Nvidia uses A-F, ST uses # and ports, Xilinx uses #s and banks. Etc.
 */
static int cmd_gpio(int argc, const cmd_args *argv)
{
    if (argc == 4 && !strcmp(argv[1].str,"set")) {
        unsigned int gpio = argv[2].u;
        unsigned int value = argv[3].u;

        gpio_set(gpio, value);
    } else if (argc == 3 && !strcmp(argv[1].str,"get")) {
        unsigned int gpio = argv[2].u;

        printf("gpio %u: %d\n", gpio, gpio_get(gpio));
    } else if (argc >= 3 && !strcmp(argv[1].str,"cfg")) {
        unsigned int gpio = argv[2].u;
        unsigned int flags = 0;

        for (int i = 3; i < argc; i++) {
            flags |= get_flag_value(argv[i].str);
        }

        gpio_config(gpio, flags);
    } else {
        printf("gpio set <gpio #> <value>\n");
        printf("gpio get <gpio #>\n");
        printf("gpio cfg <gpio #> [flags: ");
        for (unsigned int i = 0; i < countof(gpio_flag_labels); i++) {
            printf("%s ", gpio_flag_labels[i].label);
        }
        putchar(']');
        putchar('\n');
    }

    return 0;
}
STATIC_COMMAND_START
STATIC_COMMAND("gpio", "commands for manipulating system gpios", &cmd_gpio)
STATIC_COMMAND_END(gpio);

#endif
