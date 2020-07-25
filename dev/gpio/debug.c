/*
 * Copyright (c) 2015 Christopher Anderson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <dev/gpio.h>
#include <lk/console_cmd.h>
#include <string.h>
#include <stdio.h>

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

static unsigned int get_flag_value(const char *str) {
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
static int cmd_gpio(int argc, const console_cmd_args *argv) {
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
