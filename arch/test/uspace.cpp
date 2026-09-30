/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/*
 * Runs code in user mode: a stub is copied into a user address space, a thread
 * enters it with arch_enter_uspace(), and the arch's user exception hooks,
 * overridden here, record what came back and end the thread. Covers the
 * lower-EL/U-mode entry and exit paths, the syscall hook, and a user access to
 * kernel memory being caught rather than crashing the kernel.
 */

#if ARCH_ARM64 || (ARCH_RISCV && RISCV_S_MODE && RISCV_MMU)

#include <arch.h>
#include <arch/mmu.h>
#include <arch/ops.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lib/unittest.h>
#include <lk/err.h>
#include <lktl/auto_call.h>
#include <string.h>

#if ARCH_ARM64
#include <arch/arm64.h>
#include <lk/bits.h>
#else
#include <arch/riscv.h>
#include <arch/riscv/iframe.h>
#endif

extern "C" const uint8_t uspace_stub_syscall_start[], uspace_stub_syscall_end[];
extern "C" const uint8_t uspace_stub_fault_start[], uspace_stub_fault_end[];

namespace {

constexpr ulong kSyscallMagic = 0x1234;
constexpr size_t kUserStackSize = 4 * PAGE_SIZE;

// what the hook overrides below hand back to the test
volatile bool test_active;
volatile int syscalls;
volatile ulong syscall_arg;
volatile int faults;
volatile ulong fault_cause; // arm64: ESR, riscv: scause
volatile ulong fault_addr;  // arm64: FAR, riscv: stval

struct uspace_thread_args {
    vmm_aspace_t *aspace;
    vaddr_t entry;
    size_t code_len;
    vaddr_t stack_top;
};

int uspace_thread(void *arg) {
    auto *args = static_cast<uspace_thread_args *>(arg);

    vmm_set_active_aspace(args->aspace);

    // The stub was written through the kernel's physmap. Bring the instruction
    // side of this cpu in line at the address user space fetches it from; the
    // thread is pinned, so this is the cpu that will run it.
#if ARCH_ARM64
    arch_sync_cache_range(args->entry, args->code_len);
#else
    __asm__ volatile("fence.i" ::: "memory");
#endif

    arch_enter_uspace(args->entry, args->stack_top);
    __UNREACHABLE;
}

// Run one stub in a fresh user aspace on its own thread; the hooks end the
// thread and retcode is what they passed to thread_exit().
bool run_user_stub(const uint8_t *stub_start, const uint8_t *stub_end, int *retcode) {
    BEGIN_TEST;

    const size_t len = stub_end - stub_start;
    ASSERT_LE(len, (size_t)PAGE_SIZE, "stub fits in a page");

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "uspace_test", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() { vmm_free_aspace(as); });

    void *code = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_alloc(as, "code", PAGE_SIZE, &code, 0, 0,
                                  ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_EXECUTE),
              "alloc code");
    void *stack = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_alloc(as, "stack", kUserStackSize, &stack, 0, 0,
                                  ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE),
              "alloc stack");

    // the aspace is not active on this thread, so fill the code page through the physmap
    paddr_t pa = 0;
    ASSERT_EQ(NO_ERROR, arch_mmu_query(&as->arch_aspace, (vaddr_t)code, &pa, nullptr), "query code page");
    memcpy(paddr_to_kvaddr(pa), stub_start, len);

    uspace_thread_args args = {
        .aspace = as,
        .entry = (vaddr_t)code,
        .code_len = len,
        .stack_top = (vaddr_t)stack + kUserStackSize,
    };
    thread_t *t = thread_create("uspace_test", uspace_thread, &args, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "thread_create");
    thread_set_pinned_cpu(t, arch_curr_cpu_num());
    thread_resume(t);
    ASSERT_EQ(NO_ERROR, thread_join(t, retcode, 5000), "join");

    // the thread is gone, so no cpu has the aspace loaded any more
    aspace_cleanup.cancel();
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free aspace");

    END_TEST;
}

bool user_syscall() {
    BEGIN_TEST;

    syscalls = 0;
    syscall_arg = 0;
    faults = 0;
    test_active = true;
    auto done = lk::make_auto_call([]() { test_active = false; });

    int retcode = -1;
    ASSERT_TRUE(run_user_stub(uspace_stub_syscall_start, uspace_stub_syscall_end, &retcode), "run stub");
    EXPECT_EQ(0, retcode, "exit code set by the syscall hook");
    EXPECT_EQ(1, syscalls, "syscalls seen");
    EXPECT_EQ(kSyscallMagic, syscall_arg, "syscall argument");
    EXPECT_EQ(0, faults, "faults seen");

    END_TEST;
}

bool user_fault() {
    BEGIN_TEST;

    syscalls = 0;
    faults = 0;
    fault_cause = 0;
    fault_addr = 0;
    test_active = true;
    auto done = lk::make_auto_call([]() { test_active = false; });

    int retcode = -1;
    ASSERT_TRUE(run_user_stub(uspace_stub_fault_start, uspace_stub_fault_end, &retcode), "run stub");
    EXPECT_EQ(1, retcode, "exit code set by the fault hook");
    EXPECT_EQ(1, faults, "faults seen");
    EXPECT_EQ(0, syscalls, "syscalls seen");
    EXPECT_EQ((ulong)KERNEL_ASPACE_BASE, fault_addr, "faulting address");
#if ARCH_ARM64
    EXPECT_EQ(0b100100U, BITS_SHIFT(fault_cause, 31, 26), "data abort from a lower EL");
#else
    EXPECT_EQ((ulong)RISCV_EXCEPTION_LOAD_PAGE_FAULT, fault_cause, "load page fault");
#endif

    END_TEST;
}

} // namespace

// The hooks are weak in the arch; these strong definitions take over for the
// whole image, so they hand anything outside the test back to the defaults.
#if ARCH_ARM64

void arm64_syscall(struct arm64_iframe_long *iframe, bool is_64bit) {
    if (!test_active) {
        arm64_syscall_unhandled(iframe, is_64bit);
    }
    syscalls = syscalls + 1;
    syscall_arg = iframe->r[0];
    thread_exit(0);
}

void arm64_user_exception(struct arm64_iframe_long *iframe, uint32_t esr, uint64_t far) {
    if (!test_active) {
        arm64_user_exception_unhandled(iframe, esr, far);
    }
    faults = faults + 1;
    fault_cause = esr;
    fault_addr = far;
    thread_exit(1);
}

#else

void riscv_syscall_handler(struct riscv_short_iframe *frame) {
    if (!test_active) {
        riscv_syscall_unhandled(frame);
    }
    syscalls = syscalls + 1;
    syscall_arg = frame->a0;
    thread_exit(0);
}

void riscv_user_exception(long cause, ulong epc, struct riscv_short_iframe *frame) {
    if (!test_active) {
        riscv_user_exception_unhandled(cause, epc, frame);
    }
    faults = faults + 1;
    fault_cause = cause;
    fault_addr = riscv_csr_read(RISCV_CSR_XTVAL);
    thread_exit(1);
}

#endif

BEGIN_TEST_CASE(arch_uspace_tests)
RUN_TEST(user_syscall);
RUN_TEST(user_fault);
END_TEST_CASE(arch_uspace_tests)

#endif // ARCH_ARM64 || (ARCH_RISCV && RISCV_S_MODE && RISCV_MMU)
