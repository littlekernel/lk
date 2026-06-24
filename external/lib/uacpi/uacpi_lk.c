// Copyright 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lk/console_cmd.h>
#include <platform/time.h>

#include <arch/interrupts.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>

#include <kernel/vm.h>

#if ARCH_X86
#include <arch/x86.h>
#include <arch/x86/mmu.h>
#include <hw/multiboot.h>
#endif

#include <uacpi/kernel_api.h>
#include <uacpi/status.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>

#define LOCAL_TRACE 0

static bool validate_rsdp(const uint8_t *rsdp) {
    if (memcmp("RSD PTR ", rsdp, 8) != 0) {
        return false;
    }
    uint8_t sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += rsdp[i];
    }
    if (sum != 0) {
        return false;
    }
    uint8_t revision = rsdp[15];
    if (revision >= 2) {
        uint32_t length = *(const uint32_t *)(rsdp + 20);
        if (length < 36 || length > 4096) {
            return false;
        }
        sum = 0;
        for (uint32_t i = 0; i < length; i++) {
            sum += rsdp[i];
        }
        if (sum != 0) {
            return false;
        }
    }
    return true;
}

#if ARCH_X86
extern uint32_t _multiboot2_info;

static void *map_region(paddr_t pa, size_t len, const char *name) {
    const paddr_t pa_page_aligned = ROUNDDOWN(pa, PAGE_SIZE);
    const size_t align_offset = pa - pa_page_aligned;
    size_t map_len = ROUNDUP(len + align_offset, PAGE_SIZE);

    uint perms = ARCH_MMU_FLAG_PERM_RO;
    void *ptr;
    status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), name, map_len, &ptr, 0,
                                      pa_page_aligned, 0, perms);
    if (err < 0) {
        return NULL;
    }
    return (void *)((uintptr_t)ptr + align_offset);
}

static uacpi_status find_rsdp_multiboot(uacpi_phys_addr *out_rsdp_address) {
    if (_multiboot2_info != 0) {
        const paddr_t multiboot_phys = (paddr_t)_multiboot2_info;
        const void *hdr_map = map_region(multiboot_phys, PAGE_SIZE, "uacpi rsdp hdr map (mb2)");
        if (hdr_map) {
            const struct multiboot2_info *multiboot_hdr = (const struct multiboot2_info *)hdr_map;
            uint32_t total_size = multiboot_hdr->total_size;

            if (total_size >= sizeof(*multiboot_hdr) && total_size <= 1024 * 1024) {
                vmm_free_region(vmm_get_kernel_aspace(), ROUNDDOWN((vaddr_t)hdr_map, PAGE_SIZE));
                const void *rsdp_map =
                    map_region(multiboot_phys, total_size, "uacpi rsdp map (mb2)");
                if (rsdp_map) {
                    uint8_t *base = (uint8_t *)rsdp_map;
                    uint8_t *end = base + total_size;
                    uint8_t *ptr = base + sizeof(struct multiboot2_info);

                    while (ptr + sizeof(struct multiboot2_tag) <= end) {
                        struct multiboot2_tag *tag = (struct multiboot2_tag *)ptr;

                        if (tag->size < sizeof(struct multiboot2_tag) || ptr + tag->size > end) {
                            break;
                        }

                        if (tag->type == MULTIBOOT2_TAG_TYPE_ACPI_NEW ||
                            tag->type == MULTIBOOT2_TAG_TYPE_ACPI_OLD) {
                            uint8_t *rsdp_ptr = (uint8_t *)(tag + 1);
                            if (validate_rsdp(rsdp_ptr)) {
                                *out_rsdp_address = multiboot_phys + (rsdp_ptr - base);
                                vmm_free_region(vmm_get_kernel_aspace(),
                                                ROUNDDOWN((vaddr_t)rsdp_map, PAGE_SIZE));
                                return UACPI_STATUS_OK;
                            }
                        }

                        size_t advance = (tag->size + 7) & ~7;
                        ptr += advance;
                    }
                    vmm_free_region(vmm_get_kernel_aspace(),
                                    ROUNDDOWN((vaddr_t)rsdp_map, PAGE_SIZE));
                }
            } else {
                vmm_free_region(vmm_get_kernel_aspace(), ROUNDDOWN((vaddr_t)hdr_map, PAGE_SIZE));
            }
        }
    }
    return UACPI_STATUS_NOT_FOUND;
}

static uacpi_status find_rsdp_bios(uacpi_phys_addr *out_rsdp_address) {
    const paddr_t range_start = 0xe0000;
    const paddr_t range_end = 0x100000;
    const size_t len = range_end - range_start;

    const uint8_t *bios_ptr = NULL;
    status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), "uacpi rsdp bios area", len,
                                      (void **)&bios_ptr, 0, range_start, 0, ARCH_MMU_FLAG_PERM_RO);
    if (err < 0) {
        return UACPI_STATUS_NOT_FOUND;
    }

    for (size_t i = 0; i < len; i += 16) {
        if (validate_rsdp(bios_ptr + i)) {
            *out_rsdp_address = range_start + i;
            vmm_free_region(vmm_get_kernel_aspace(), (vaddr_t)bios_ptr);
            return UACPI_STATUS_OK;
        }
    }

    vmm_free_region(vmm_get_kernel_aspace(), (vaddr_t)bios_ptr);
    return UACPI_STATUS_NOT_FOUND;
}
#endif

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    if (!out_rsdp_address) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
#if ARCH_X86
    uacpi_status ret = find_rsdp_multiboot(out_rsdp_address);
    if (ret == UACPI_STATUS_OK) {
        return UACPI_STATUS_OK;
    }
    return find_rsdp_bios(out_rsdp_address);
#else
    return UACPI_STATUS_NOT_FOUND;
#endif
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    void *vaddr = paddr_to_kvaddr(addr);
    if (vaddr) {
        return vaddr;
    }

    // Not in direct map, allocate virtual mapping
    paddr_t pa_aligned = ROUNDDOWN((paddr_t)addr, PAGE_SIZE);
    size_t align_offset = (paddr_t)addr - pa_aligned;
    size_t map_len = ROUNDUP(len + align_offset, PAGE_SIZE);

    uint perms = ARCH_MMU_FLAG_PERM_RO;
    if (arch_mmu_supports_nx_mappings()) {
        perms |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
    }

    void *ptr = NULL;
    status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), "uacpi_map", map_len, &ptr, 0,
                                      pa_aligned, 0, perms);
    if (err < 0) {
        return UACPI_MAP_FAILED;
    }

    return (void *)((uintptr_t)ptr + align_offset);
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    // Only call vmm_free_region if the mapping was dynamically allocated.
    // Direct-mapped addresses must not be freed since they reside inside the permanent "physmap"
    // region.
    paddr_t pa = vaddr_to_paddr(addr);
    if (pa) {
        void *direct_vaddr = paddr_to_kvaddr(pa);
        if (direct_vaddr == addr) {
            return;
        }
    }

    vmm_free_region(vmm_get_kernel_aspace(), ROUNDDOWN((vaddr_t)addr, PAGE_SIZE));
}

void uacpi_kernel_log(uacpi_log_level level, const uacpi_char *str) {
    int lk_level = INFO;
    if (level == UACPI_LOG_DEBUG) {
        lk_level = SPEW;
    } else if (level == UACPI_LOG_INFO) {
        lk_level = INFO;
    } else if (level == UACPI_LOG_WARN || level == UACPI_LOG_ERROR) {
        lk_level = ALWAYS;
    }
    dprintf(lk_level, "uACPI: %s", str);
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return malloc(size);
}

void uacpi_kernel_free(void *mem) {
    free(mem);
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    return (uacpi_u64)current_time_hires() * 1000;
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    spin(usec);
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    thread_sleep(msec);
}

uacpi_handle uacpi_kernel_create_mutex(void) {
    mutex_t *mutex = malloc(sizeof(mutex_t));
    if (mutex) {
        mutex_init(mutex);
    }
    return (uacpi_handle)mutex;
}

void uacpi_kernel_free_mutex(uacpi_handle handle) {
    mutex_t *mutex = (mutex_t *)handle;
    if (mutex) {
        mutex_destroy(mutex);
        free(mutex);
    }
}

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 timeout) {
    mutex_t *mutex = (mutex_t *)handle;
    if (!mutex) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
    lk_time_t lk_timeout = (timeout == 0xffff) ? INFINITE_TIME : timeout;
    status_t status = mutex_acquire_timeout(mutex, lk_timeout);
    if (status == NO_ERROR) {
        return UACPI_STATUS_OK;
    }
    if (status == ERR_TIMED_OUT) {
        return UACPI_STATUS_TIMEOUT;
    }
    return UACPI_STATUS_INTERNAL_ERROR;
}

void uacpi_kernel_release_mutex(uacpi_handle handle) {
    mutex_t *mutex = (mutex_t *)handle;
    if (mutex) {
        mutex_release(mutex);
    }
}

uacpi_handle uacpi_kernel_create_event(void) {
    semaphore_t *sem = malloc(sizeof(semaphore_t));
    if (sem) {
        sem_init(sem, 0);
    }
    return (uacpi_handle)sem;
}

void uacpi_kernel_free_event(uacpi_handle handle) {
    semaphore_t *sem = (semaphore_t *)handle;
    if (sem) {
        sem_destroy(sem);
        free(sem);
    }
}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout) {
    semaphore_t *sem = (semaphore_t *)handle;
    if (!sem) {
        return UACPI_FALSE;
    }
    lk_time_t lk_timeout = (timeout == 0xffff) ? INFINITE_TIME : timeout;
    status_t status = sem_timedwait(sem, lk_timeout);
    return (status == NO_ERROR) ? UACPI_TRUE : UACPI_FALSE;
}

void uacpi_kernel_signal_event(uacpi_handle handle) {
    semaphore_t *sem = (semaphore_t *)handle;
    if (sem) {
        sem_post(sem, false);
    }
}

void uacpi_kernel_reset_event(uacpi_handle handle) {
    semaphore_t *sem = (semaphore_t *)handle;
    if (sem) {
        THREAD_LOCK(state);
        if (sem->count > 0) {
            sem->count = 0;
        }
        spin_unlock_irqrestore(&thread_lock, state);
    }
}

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return (uacpi_thread_id)get_current_thread();
}

uacpi_interrupt_state uacpi_kernel_disable_interrupts(void) {
    arch_interrupt_saved_state_t state = arch_interrupt_save();
    uacpi_interrupt_state val = 0;
    STATIC_ASSERT(sizeof(state) <= sizeof(val));
    memcpy(&val, &state, sizeof(state));
    return val;
}

void uacpi_kernel_restore_interrupts(uacpi_interrupt_state state_val) {
    arch_interrupt_saved_state_t state;
    STATIC_ASSERT(sizeof(state) <= sizeof(state_val));
    memcpy(&state, &state_val, sizeof(state));
    arch_interrupt_restore(state);
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
    spin_lock_t *lock = malloc(sizeof(spin_lock_t));
    if (lock) {
        spin_lock_init(lock);
    }
    return (uacpi_handle)lock;
}

void uacpi_kernel_free_spinlock(uacpi_handle handle) {
    free(handle);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle) {
    spin_lock_t *lock = (spin_lock_t *)handle;
    arch_interrupt_saved_state_t state = spin_lock_irqsave(lock);
    uacpi_cpu_flags flags = 0;
    STATIC_ASSERT(sizeof(state) <= sizeof(flags));
    memcpy(&flags, &state, sizeof(state));
    return flags;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags) {
    spin_lock_t *lock = (spin_lock_t *)handle;
    arch_interrupt_saved_state_t state;
    STATIC_ASSERT(sizeof(state) <= sizeof(flags));
    memcpy(&state, &flags, sizeof(state));
    spin_unlock_irqrestore(lock, state);
}

uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address, uacpi_handle *out_handle) {
    return UACPI_STATUS_NOT_FOUND;
}

void uacpi_kernel_pci_device_close(uacpi_handle handle) {}

uacpi_status uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset, uacpi_u8 *value) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset, uacpi_u16 *value) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset, uacpi_u32 *value) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset, uacpi_u8 value) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset, uacpi_u16 value) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset, uacpi_u32 value) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle) {
    *out_handle = (uacpi_handle)(uintptr_t)base;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {}

uacpi_status uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value) {
    uacpi_io_addr addr = (uacpi_io_addr)(uintptr_t)handle + offset;
#if ARCH_X86
    *out_value = inp((uint16_t)addr);
    return UACPI_STATUS_OK;
#else
    return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value) {
    uacpi_io_addr addr = (uacpi_io_addr)(uintptr_t)handle + offset;
#if ARCH_X86
    *out_value = inpw((uint16_t)addr);
    return UACPI_STATUS_OK;
#else
    return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value) {
    uacpi_io_addr addr = (uacpi_io_addr)(uintptr_t)handle + offset;
#if ARCH_X86
    *out_value = inpd((uint16_t)addr);
    return UACPI_STATUS_OK;
#else
    return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value) {
    uacpi_io_addr addr = (uacpi_io_addr)(uintptr_t)handle + offset;
#if ARCH_X86
    outp((uint16_t)addr, in_value);
    return UACPI_STATUS_OK;
#else
    return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value) {
    uacpi_io_addr addr = (uacpi_io_addr)(uintptr_t)handle + offset;
#if ARCH_X86
    outpw((uint16_t)addr, in_value);
    return UACPI_STATUS_OK;
#else
    return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value) {
    uacpi_io_addr addr = (uacpi_io_addr)(uintptr_t)handle + offset;
#if ARCH_X86
    outpd((uint16_t)addr, in_value);
    return UACPI_STATUS_OK;
#else
    return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler handler,
                                                    uacpi_handle ctx,
                                                    uacpi_handle *out_irq_handle) {
    *out_irq_handle = (uacpi_handle)1;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler handler,
                                                      uacpi_handle irq_handle) {
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler handler,
                                        uacpi_handle ctx) {
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    return UACPI_STATUS_OK;
}

static int cmd_uacpi(int argc, const console_cmd_args *argv) {
    printf("Initializing uACPI...\n");
    uacpi_status status = uacpi_initialize(0);
    if (status != UACPI_STATUS_OK) {
        printf("uacpi_initialize failed: %s (%d)\n", uacpi_status_to_string(status), status);
        return -1;
    }

    printf("Loading tables...\n");
    status = uacpi_namespace_load();
    if (status != UACPI_STATUS_OK) {
        printf("uacpi_namespace_load failed: %s (%d)\n", uacpi_status_to_string(status), status);
        uacpi_state_reset();
        return -1;
    }

    printf("Initializing namespace...\n");
    status = uacpi_namespace_initialize();
    if (status != UACPI_STATUS_OK) {
        printf("uacpi_namespace_initialize failed: %s (%d)\n", uacpi_status_to_string(status),
               status);
        uacpi_state_reset();
        return -1;
    }

    printf("uACPI Initialized successfully.\n");
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("uacpi", "Initialize and test uACPI", &cmd_uacpi)
STATIC_COMMAND_END(uacpi);
