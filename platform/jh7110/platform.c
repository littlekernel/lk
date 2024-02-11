/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <inttypes.h>
#include <lk/err.h>
#include <lk/main.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/jh7110.h>
#include <sys/types.h>
#include <lib/fdtwalk.h>
#include <dev/interrupt/riscv_plic.h>
#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif
#include <kernel/vm.h>
#if WITH_LIB_CONSOLE
#include <lib/console.h>
#endif

#include "platform_p.h"

#define LOCAL_TRACE 1

#if WITH_KERNEL_VM
#define DEFAULT_MEMORY_SIZE (MEMSIZE) /* try to fetch from the emulator via the fdt */

static pmm_arena_t arena = {
    .name = "ram",
    .base = MEMORY_BASE_PHYS,
    .size = DEFAULT_MEMORY_SIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};
#endif

static volatile uint32_t *power_reset_reg;

// callbacks to the fdt_walk routine
static void memcallback(uint64_t base, uint64_t len, void *cookie) {
    bool *found_mem = (bool *)cookie;

    LTRACEF("base %#llx len %#llx cookie %p\n", base, len, cookie);

    /* add another vm arena */
    if (!*found_mem) {
        printf("FDT: found memory arena, base %#llx size %#llx\n", base, len);
#if WITH_KERNEL_VM
        arena.base = base;
        arena.size = len;
        pmm_add_arena(&arena);
#else
        novm_add_arena("fdt", base, len);
#endif
        *found_mem = true; // stop searching after the first one
    }
}

struct reserved_memory_regions {
    size_t count;

    struct {
        uint64_t base;
        uint64_t len;
    } regions[16];
};

static void reserved_memory_callback(uint64_t base, uint64_t len, void *cookie) {
    struct reserved_memory_regions *mem = cookie;

    LTRACEF("base %#llx len %#llx\n", base, len);

    if (mem->count < countof(mem->regions)) {
        mem->regions[mem->count].base = base;
        mem->regions[mem->count].len = len;
        mem->count++;
    }
}

static void cpucallback(uint64_t id, void *cookie) {
    int *cpu_count = (int *)cookie;

    LTRACEF("id %#llx cookie %p\n", id, cookie);

    (*cpu_count)++;
}

struct pcie_detect_state {
    struct fdt_walk_pcie_info info;
} pcie_state;

static void pciecallback(const struct fdt_walk_pcie_info *info, void *cookie) {
    struct pcie_detect_state *state = cookie;

    LTRACEF("ecam base %#llx, len %#llx, bus_start %hhu, bus_end %hhu\n", info->ecam_base, info->ecam_len, info->bus_start, info->bus_end);
    state->info = *info;
}

void platform_early_init(void) {
    TRACE;
    plic_early_init(PLIC_BASE_VIRT, NUM_IRQS, true);

    LTRACEF("starting FDT scan\n");

    /* look for a flattened device tree in the second arg passed to us */
    bool found_mem = false;
    int cpu_count = 0;
    struct reserved_memory_regions reserved = {0};

    const void *fdt = (void *)lk_boot_args[1];
#if WITH_KERNEL_VM
    fdt = (const void *)((uintptr_t)fdt + KERNEL_ASPACE_BASE);
#endif

    struct fdt_walk_callbacks cb = {
        .mem = memcallback,
        .memcookie = &found_mem,
        .reserved_memory = reserved_memory_callback,
        .reserved_memory_cookie = &reserved,
        .cpu = cpucallback,
        .cpucookie = &cpu_count,
        .pcie = pciecallback,
        .pciecookie = &pcie_state,
    };

    status_t err = fdt_walk(fdt, &cb);
    LTRACEF("fdt_walk returns %d\n", err);

    if (err != 0) {
        printf("FDT: error finding FDT at %p, using default memory & cpu count\n", fdt);
    }

    // Always reserve all of physical memory below the kernel, this is where SBI lives
    // TODO: figure out why uboot doesn't always put this here
    reserved.regions[reserved.count].base = MEMBASE;
    reserved.regions[reserved.count].len = KERNEL_LOAD_OFFSET;
    reserved.count++;

    /* add a default memory region if we didn't find it in the FDT */
    if (!found_mem) {
#if WITH_KERNEL_VM
        pmm_add_arena(&arena);
#else
        novm_add_arena("default", MEMBASE, MEMSIZE);
#endif
    }

#if WITH_KERNEL_VM
    /* reserve memory described by the FDT */
    for (size_t i = 0; i < reserved.count; i++) {
        printf("FDT: reserving memory range [%#llx ... %#llx]\n",
                reserved.regions[i].base, reserved.regions[i].base + reserved.regions[i].len - 1);
        struct list_node list = LIST_INITIAL_VALUE(list);
        pmm_alloc_range(reserved.regions[i].base, reserved.regions[i].len / PAGE_SIZE, &list);
    }
#endif

    if (cpu_count > 0) {
        printf("FDT: found %d cpu%c\n", cpu_count, cpu_count == 1 ? ' ' : 's');
        riscv_set_secondary_count(cpu_count - 1);
    } else {
        riscv_set_secondary_count(0);
    }

    LTRACEF("done scanning FDT\n");

    /* save a copy of the pointer to the poweroff/reset register */
    /* TODO: read it from the FDT */
#if WITH_KERNEL_VM
    power_reset_reg = paddr_to_kvaddr(0x100000);
#else
    power_reset_reg = (void *)0x100000;
#endif
}

void platform_init(void) {
    plic_init();
    uart_init();
}

void platform_halt(platform_halt_action suggested_action,
                          platform_halt_reason reason) {
    switch (suggested_action) {
        case HALT_ACTION_SHUTDOWN:
            dprintf(ALWAYS, "Shutting down... (reason = %d)\n", reason);
#if RISCV_S_MODE
            // try to use SBI as a cleaner way to stop
            sbi_system_reset(SBI_RESET_TYPE_SHUTDOWN, SBI_RESET_REASON_NONE);
#endif
            *power_reset_reg = 0x5555;
            break;
        case HALT_ACTION_REBOOT:
            dprintf(ALWAYS, "Rebooting... (reason = %d)\n", reason);
#if RISCV_S_MODE
            sbi_system_reset(SBI_RESET_TYPE_COLD_REBOOT, SBI_RESET_REASON_NONE);
#endif
            *power_reset_reg = 0x7777;
            break;
        case HALT_ACTION_HALT:
#if ENABLE_PANIC_SHELL
            if (reason == HALT_REASON_SW_PANIC) {
                dprintf(ALWAYS, "CRASH: starting debug shell... (reason = %d)\n", reason);
                arch_disable_ints();
                panic_shell_start();
            }
#endif  // ENABLE_PANIC_SHELL
            dprintf(ALWAYS, "HALT: spinning forever... (reason = %d)\n", reason);
            break;
    }

    arch_disable_ints();
    for (;;)
        arch_idle();
}
