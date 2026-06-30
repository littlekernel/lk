#include <lk/trace.h>
#include <lk/debug.h>
#include <arch.h>
#include <platform.h>

void arch_early_init(void) {
}

void arch_init(void) {
}

void arch_idle(void) {
#if 0
    /* In SPARC, entering idle state can be done by setting a power-saving register
     * or executing a specific instruction if supported. For now, just nop. */
    __asm__ volatile("nop");
#endif
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}

void arch_disable_cache(uint flags) {
    PANIC_UNIMPLEMENTED;
}

void arch_enable_cache(uint flags) {
    PANIC_UNIMPLEMENTED;
}

void arch_clean_cache_range(addr_t start, size_t len) {
    PANIC_UNIMPLEMENTED;
}

void arch_clean_invalidate_cache_range(addr_t start, size_t len) {
    PANIC_UNIMPLEMENTED;
}

void arch_invalidate_cache_range(addr_t start, size_t len) {
    PANIC_UNIMPLEMENTED;
}

void arch_sync_cache_range(addr_t start, size_t len) {
    PANIC_UNIMPLEMENTED;
}
