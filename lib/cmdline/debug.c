/*
 * Copyright 2026 The LK Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <ctype.h>
#include <inttypes.h>
#include <lib/cmdline.h>
#include <lk/console_cmd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void cmdline_print_escaped(const char *buf, size_t len) {
    putchar('"');
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)buf[i];
        switch (c) {
            case '\\':
                printf("\\\\");
                break;
            case '"':
                printf("\\\"");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\r':
                printf("\\r");
                break;
            case '\t':
                printf("\\t");
                break;
            case '\0':
                printf("\\0");
                break;
            default:
                if (isprint(c)) {
                    putchar(c);
                } else {
                    printf("\\x%02x", c);
                }
                break;
        }
    }
    putchar('"');
}

static int cmdline_usage(const console_cmd_args *argv, const char *usage) {
    printf("usage: %s %s\n", argv[0].str, usage);
    return -1;
}

static int cmdline_print_not_initialized(const console_cmd_args *argv) {
    printf("%s: command line not initialized\n", argv[0].str);
    return -1;
}

static int cmdline_print_query_error(const char *var, status_t st) {
    switch (st) {
        case ERR_NOT_FOUND:
            printf("%s: not found\n", var);
            break;
        case ERR_INVALID_ARGS:
            printf("%s: invalid value\n", var);
            break;
        case ERR_NOT_ENOUGH_BUFFER:
            printf("%s: output buffer too small\n", var);
            break;
        default:
            printf("%s: error %d\n", var, st);
            break;
    }
    return -1;
}

static int cmdline_cmd_show(int argc, const console_cmd_args *argv) {
    if (argc != 1) {
        return cmdline_usage(argv, "");
    }

    const char *raw = cmdline_get_raw_string();
    size_t len = cmdline_get_raw_size();

    if (!raw) {
        return cmdline_print_not_initialized(argv);
    }

    printf("cmdline (%zu bytes) = ", len);
    cmdline_print_escaped(raw, len);
    putchar('\n');
    return 0;
}

static int cmdline_cmd_present(int argc, const console_cmd_args *argv) {
    if (argc != 2) {
        return cmdline_usage(argv, "<var>");
    }

    const char *var = argv[1].str;

    if (!cmdline_get_raw_string()) {
        return cmdline_print_not_initialized(argv);
    }

    bool present = cmdline_is_present(var);
    printf("%s: %s\n", var, present ? "present" : "not present");
    return present ? 0 : -1;
}

static int cmdline_cmd_value(int argc, const console_cmd_args *argv) {
    if (argc != 2) {
        return cmdline_usage(argv, "<var>");
    }

    const char *var = argv[1].str;
    const char *value;
    size_t len;

    if (!cmdline_get_raw_string()) {
        return cmdline_print_not_initialized(argv);
    }

    status_t st = cmdline_get_value(var, &value, &len);
    if (st != NO_ERROR) {
        return cmdline_print_query_error(var, st);
    }

    printf("%s: ", var);
    cmdline_print_escaped(value, len);
    printf(" (len=%zu)\n", len);
    return 0;
}

static int cmdline_cmd_string(int argc, const console_cmd_args *argv) {
    if (argc != 2) {
        return cmdline_usage(argv, "<var>");
    }

    const char *var = argv[1].str;
    const char *raw;
    size_t raw_len;

    if (!cmdline_get_raw_string()) {
        return cmdline_print_not_initialized(argv);
    }

    status_t st = cmdline_get_value(var, &raw, &raw_len);
    if (st != NO_ERROR) {
        return cmdline_print_query_error(var, st);
    }

    char *buf = malloc(raw_len + 1);
    if (!buf) {
        printf("%s: out of memory\n", var);
        return -1;
    }

    size_t out_len = 0;
    st = cmdline_get_string(var, buf, raw_len + 1, &out_len);
    if (st != NO_ERROR) {
        free(buf);
        return cmdline_print_query_error(var, st);
    }

    printf("%s: ", var);
    cmdline_print_escaped(buf, out_len);
    printf(" (len=%zu)\n", out_len);
    free(buf);
    return 0;
}

static int cmdline_cmd_bool(int argc, const console_cmd_args *argv) {
    if (argc != 2) {
        return cmdline_usage(argv, "<var>");
    }

    const char *var = argv[1].str;
    bool value;

    if (!cmdline_get_raw_string()) {
        return cmdline_print_not_initialized(argv);
    }

    status_t st = cmdline_get_bool(var, &value);
    if (st != NO_ERROR) {
        return cmdline_print_query_error(var, st);
    }

    printf("%s: %s\n", var, value ? "true" : "false");
    return 0;
}

static int cmdline_cmd_u32(int argc, const console_cmd_args *argv) {
    if (argc != 2) {
        return cmdline_usage(argv, "<var>");
    }

    const char *var = argv[1].str;
    uint32_t value;

    if (!cmdline_get_raw_string()) {
        return cmdline_print_not_initialized(argv);
    }

    status_t st = cmdline_get_uint32(var, &value);
    if (st != NO_ERROR) {
        return cmdline_print_query_error(var, st);
    }

    printf("%s: %" PRIu32 " (0x%" PRIx32 ")\n", var, value, value);
    return 0;
}

static int cmdline_cmd_u64(int argc, const console_cmd_args *argv) {
    if (argc != 2) {
        return cmdline_usage(argv, "<var>");
    }

    const char *var = argv[1].str;
    uint64_t value;

    if (!cmdline_get_raw_string()) {
        return cmdline_print_not_initialized(argv);
    }

    status_t st = cmdline_get_uint64(var, &value);
    if (st != NO_ERROR) {
        return cmdline_print_query_error(var, st);
    }

    printf("%s: %" PRIu64 " (0x%" PRIx64 ")\n", var, value, value);
    return 0;
}

static int cmdline_cmd_int(int argc, const console_cmd_args *argv) {
    if (argc != 2) {
        return cmdline_usage(argv, "<var>");
    }

    const char *var = argv[1].str;
    int value;

    if (!cmdline_get_raw_string()) {
        return cmdline_print_not_initialized(argv);
    }

    status_t st = cmdline_get_int(var, &value);
    if (st != NO_ERROR) {
        return cmdline_print_query_error(var, st);
    }

    printf("%s: %d\n", var, value);
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("cmdline", "print the current kernel command line", &cmdline_cmd_show)
STATIC_COMMAND("cmdline_has", "check whether a command line variable is present",
               &cmdline_cmd_present)
STATIC_COMMAND("cmdline_value", "print the raw command line value for a variable",
               &cmdline_cmd_value)
STATIC_COMMAND("cmdline_string", "print a command line value as an unescaped string",
               &cmdline_cmd_string)
STATIC_COMMAND("cmdline_bool", "read a command line variable as a bool", &cmdline_cmd_bool)
STATIC_COMMAND("cmdline_u32", "read a command line variable as a uint32", &cmdline_cmd_u32)
STATIC_COMMAND("cmdline_u64", "read a command line variable as a uint64", &cmdline_cmd_u64)
STATIC_COMMAND("cmdline_int", "read a command line variable as an int", &cmdline_cmd_int)
STATIC_COMMAND_END(cmdline);
