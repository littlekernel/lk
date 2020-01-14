#include <sys/types.h>
#include <stdint.h>
#include <lk/debug.h>

void arch_early_init(void) {
}

void arch_init(void) {
}

void arch_idle(void) {
    asm volatile("sleep");
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}

