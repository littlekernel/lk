/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/mips.h>
#include <dev/uart.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/main.h>
#include <lk/reg.h>
#include <platform.h>
#include <platform/debug.h>
#include <platform/interrupts.h>
#include <platform/qemu-mips.h>
#include <platform/timer.h>
#include <kernel/novm.h>
#include <rand.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

extern void platform_init_interrupts(void);
extern void platform_init_uart(void);
extern void uart_init(void);

#define MAX_ENVP_DUMP 32
#define EMEM_BASE_PHYS 0x80000000UL
#define MAX_RNGSEED_BYTES 64

static uintptr_t detected_memsize = MEMSIZE; // default to the compile-time value

static bool mips_in_k0_space(const void *ptr) {
    return ((uintptr_t)ptr >= 0x80000000 && (uintptr_t)ptr < 0xa0000000);
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static status_t parse_args(void) {
    size_t argc = lk_boot_args[0];
    const char *const *envp = (const char *const *)(uintptr_t)lk_boot_args[2];

    // do a quick check to see if these args seem reasonable
    if (argc > 16) {
        dprintf(CRITICAL, "boot args: argc=%zu seems unreasonable\n", argc);
        return ERR_INVALID_ARGS;
    }

    // envp should be a valid pointer in k0 space
    if (!mips_in_k0_space(envp)) {
        dprintf(CRITICAL, "boot args: envp not in k0 space\n");
        return ERR_INVALID_ARGS;
    }

    detected_memsize = (uintptr_t)lk_boot_args[3];

    if (envp) {
        for (ulong i = 0; i < MAX_ENVP_DUMP; ++i) {
            if (!envp[i]) {
                break;
            }

            // dprintf(INFO, "envp[%lu] @ %p = '%s'\n", i, envp[i], envp[i]);

            if ((i & 1) == 0 && (i + 1) < MAX_ENVP_DUMP && envp[i + 1]) {
                // Extended memory describes the amount of *actual* memory available, mapped
                // from 0x8000.0000 physical and up. Low memory is an alias up up to 256MB
                // mapped at physical address 0 to 0x1000.0000.
                if (!strcmp(envp[i], "ememsize")) {
                    char *end = NULL;
                    unsigned long ememsize = strtoul(envp[i + 1], &end, 0);

                    if (end && *end == '\0') {
                        if (ememsize > 0) {
                            uintptr_t emem_end = EMEM_BASE_PHYS + (uintptr_t)ememsize - 1;
                            dprintf(INFO,
                                    "emem: base=0x%lx size=0x%lx end=0x%lx (not mapped)\n",
                                    (ulong)EMEM_BASE_PHYS, ememsize, (ulong)emem_end);
                        }
                    }
                }

                // Seed the RNG with any values passed to us
                if (!strcmp(envp[i], "rngseed")) {
                    const char *seed_hex = envp[i + 1];
                    size_t hex_len = strlen(seed_hex);
                    uint8_t seed_bytes[MAX_RNGSEED_BYTES];

                    if ((hex_len & 1) != 0) {
                        dprintf(CRITICAL, "rngseed length is odd: %zu\n", hex_len);
                    } else {
                        size_t byte_len = hex_len / 2;
                        if (byte_len > MAX_RNGSEED_BYTES) {
                            byte_len = MAX_RNGSEED_BYTES;
                        }

                        bool ok = true;
                        for (size_t j = 0; j < byte_len; ++j) {
                            int hi = hex_nibble(seed_hex[j * 2]);
                            int lo = hex_nibble(seed_hex[j * 2 + 1]);
                            if (hi < 0 || lo < 0) {
                                ok = false;
                                dprintf(CRITICAL, "rngseed contains non-hex characters\n");
                                break;
                            }
                            seed_bytes[j] = (uint8_t)((hi << 4) | lo);
                        }

                        if (ok && byte_len > 0) {
                            dprintf(INFO, "rngseed: adding %zu bytes of entropy\n", byte_len);
                            rand_add_entropy(seed_bytes, byte_len);
                        }
                    }
                }
            }
        }
    }

    return NO_ERROR;
}

void platform_early_init(void) {
    platform_init_interrupts();
    uart_init_early();

    parse_args();

    // For now, always use low memory size passed in a3 since we can't handle memory that's not aliased into k0.
    novm_add_arena("mem", MEMBASE, detected_memsize);

    // Clock rate is 320Mhz, timer runs at half that.
    mips_init_timer(320000000 / 2);

    // the PIC is mapped to mips irq 2
    mips_enable_irq(2);
}

void platform_init(void) {
    uart_init();
}
