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
 * lower-EL/U-mode entry and exit paths, the syscall hook, a user access to
 * kernel memory being caught rather than crashing the kernel, and
 * arch_mmu_protect() taking effect for user stores and fetches.
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
#include <kernel/mp.h>
#endif

extern "C" const uint8_t uspace_stub_syscall_start[], uspace_stub_syscall_end[];
extern "C" const uint8_t uspace_stub_fault_start[], uspace_stub_fault_end[];
extern "C" const uint8_t uspace_stub_store_start[], uspace_stub_store_target[], uspace_stub_store_end[];
extern "C" const uint8_t uspace_stub_jump_start[], uspace_stub_jump_target[], uspace_stub_jump_end[];
extern "C" const uint8_t uspace_stub_spin_start[], uspace_stub_spin_end[];

namespace {

constexpr ulong kSyscallMagic = 0x1234;
constexpr ulong kStoreMagic = 0x5678;
constexpr size_t kUserStackSize = 4 * PAGE_SIZE;
constexpr uint kUser = ARCH_MMU_FLAG_PERM_USER;
constexpr uint kR = ARCH_MMU_FLAG_PERM_READ;
constexpr uint kW = ARCH_MMU_FLAG_PERM_WRITE;
constexpr uint kX = ARCH_MMU_FLAG_PERM_EXECUTE;

// what the hook overrides below hand back to the test
volatile bool test_active;
volatile int syscalls;
volatile ulong syscall_arg;
volatile int faults;
volatile ulong fault_cause; // arm64: ESR, riscv: scause
volatile ulong fault_addr;  // arm64: FAR, riscv: stval

// A user aspace with a code page, a stack and a data page for the stubs to use.
struct user_env {
    vmm_aspace_t *as = nullptr;
    vaddr_t code = 0;
    size_t code_len = 0;
    vaddr_t stack = 0;
    vaddr_t data = 0;

    ~user_env() {
        if (as) {
            vmm_free_aspace(as);
        }
    }
};

bool user_env_setup(user_env *env) {
    BEGIN_TEST;

    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&env->as, "uspace_test", 0), "create aspace");

    void *ptr = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_alloc(env->as, "code", PAGE_SIZE, &ptr, 0, 0, kUser | kR | kX), "alloc code");
    env->code = (vaddr_t)ptr;
    ASSERT_EQ(NO_ERROR, vmm_alloc(env->as, "stack", kUserStackSize, &ptr, 0, 0, kUser | kR | kW),
              "alloc stack");
    env->stack = (vaddr_t)ptr;
    ASSERT_EQ(NO_ERROR, vmm_alloc(env->as, "data", PAGE_SIZE, &ptr, 0, 0, kUser | kR | kW), "alloc data");
    env->data = (vaddr_t)ptr;

    END_TEST;
}

// the kernel's view of one of the aspace's pages, which is not active on this thread
uint8_t *user_page(const user_env &env, vaddr_t va) {
    paddr_t pa = 0;
    if (arch_mmu_query(&env.as->arch_aspace, va, &pa, nullptr) != NO_ERROR) {
        return nullptr;
    }
    return (uint8_t *)paddr_to_kvaddr(pa);
}

// Copy a stub into the code page; target_literal, if there is one, is where in
// the stub the address it acts on goes.
bool user_env_load(user_env *env, const uint8_t *start, const uint8_t *end,
                   const uint8_t *target_literal = nullptr, vaddr_t target = 0) {
    BEGIN_TEST;

    env->code_len = end - start;
    ASSERT_LE(env->code_len, (size_t)PAGE_SIZE, "stub fits in a page");
    uint8_t *code = user_page(*env, env->code);
    ASSERT_NONNULL(code, "code page");
    memcpy(code, start, env->code_len);
    if (target_literal) {
        const uint64_t t = target;
        memcpy(code + (target_literal - start), &t, sizeof(t));
    }

    END_TEST;
}

struct uspace_thread_args {
    const user_env *env;
};

int uspace_thread(void *arg) {
    const user_env *env = static_cast<uspace_thread_args *>(arg)->env;

    vmm_set_active_aspace(env->as);

    // The stub, and whatever the data page holds, were written through the
    // kernel's physmap. Bring the instruction side of this cpu in line at the
    // addresses user space fetches them from; the thread is pinned, so this is
    // the cpu that will run it.
#if ARCH_ARM64
    arch_sync_cache_range(env->code, env->code_len);
    arch_sync_cache_range(env->data, PAGE_SIZE);
#else
    __asm__ volatile("fence.i" ::: "memory");
#endif

    arch_enter_uspace(env->code, env->stack + kUserStackSize);
    __UNREACHABLE;
}

// Run the loaded stub on its own thread, with the hooks' counts cleared; the
// hooks end the thread and retcode is what they passed to thread_exit().
bool user_env_run(const user_env &env, int *retcode) {
    BEGIN_TEST;

    syscalls = 0;
    syscall_arg = 0;
    faults = 0;
    fault_cause = 0;
    fault_addr = 0;

    uspace_thread_args args = { .env = &env };
    thread_t *t = thread_create("uspace_test", uspace_thread, &args, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "thread_create");
    thread_set_pinned_cpu(t, arch_curr_cpu_num());
    thread_resume(t);
    ASSERT_EQ(NO_ERROR, thread_join(t, retcode, 5000), "join");

    END_TEST;
}

// Run one stub in a fresh user aspace.
bool run_user_stub(const uint8_t *stub_start, const uint8_t *stub_end, int *retcode) {
    BEGIN_TEST;

    user_env env;
    ASSERT_TRUE(user_env_setup(&env), "setup");
    ASSERT_TRUE(user_env_load(&env, stub_start, stub_end), "load");
    ASSERT_TRUE(user_env_run(env, retcode), "run");

    // the thread is gone, so no cpu has the aspace loaded any more
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(env.as), "free aspace");
    env.as = nullptr;

    END_TEST;
}

bool user_syscall() {
    BEGIN_TEST;

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

// A store to the data page goes through, faults once protect takes WRITE away,
// and goes through again once it is back. A jump to the data page faults while
// it lacks EXECUTE and runs once it has it. On riscv the upgrades prove less:
// the kernel retries a fault the page table no longer explains, which would
// hide a missing invalidate.
bool user_protect() {
    BEGIN_TEST;

    test_active = true;
    auto done = lk::make_auto_call([]() { test_active = false; });

    user_env env;
    ASSERT_TRUE(user_env_setup(&env), "setup");
    arch_aspace_t *aspace = &env.as->arch_aspace;
    uint8_t *data = user_page(env, env.data);
    ASSERT_NONNULL(data, "data page");
    ASSERT_TRUE(user_env_load(&env, uspace_stub_store_start, uspace_stub_store_end,
                              uspace_stub_store_target, env.data), "load store stub");

    int retcode = -1;
    ASSERT_TRUE(user_env_run(env, &retcode), "run");
    EXPECT_EQ(0, retcode, "store to a writable page");
    EXPECT_EQ(0, faults, "faults");
    EXPECT_EQ(kStoreMagic, *(const volatile ulong *)data, "stored");

    ASSERT_EQ(NO_ERROR, arch_mmu_protect(aspace, env.data, 1, kR), "protect read only");
    ASSERT_TRUE(user_env_run(env, &retcode), "run");
    EXPECT_EQ(1, retcode, "store to a read only page");
    EXPECT_EQ(1, faults, "faults");
    EXPECT_EQ(env.data, fault_addr, "faulting address");
#if ARCH_ARM64
    EXPECT_EQ(0b100100U, BITS_SHIFT(fault_cause, 31, 26), "data abort from a lower EL");
    EXPECT_TRUE(BIT(fault_cause, 6), "on a write");
#else
    EXPECT_EQ((ulong)RISCV_EXCEPTION_STORE_PAGE_FAULT, fault_cause, "store page fault");
#endif

    ASSERT_EQ(NO_ERROR, arch_mmu_protect(aspace, env.data, 1, kR | kW), "protect writable");
    ASSERT_TRUE(user_env_run(env, &retcode), "run");
    EXPECT_EQ(0, retcode, "store once writable again");
    EXPECT_EQ(0, faults, "faults");

    ASSERT_TRUE(user_env_load(&env, uspace_stub_jump_start, uspace_stub_jump_end,
                              uspace_stub_jump_target, env.data), "load jump stub");
    ASSERT_TRUE(user_env_run(env, &retcode), "run");
    EXPECT_EQ(1, retcode, "jump to a page without execute");
    EXPECT_EQ(1, faults, "faults");
    EXPECT_EQ(env.data, fault_addr, "faulting address");
#if ARCH_ARM64
    EXPECT_EQ(0b100000U, BITS_SHIFT(fault_cause, 31, 26), "instruction abort from a lower EL");
#else
    EXPECT_EQ((ulong)RISCV_EXCEPTION_INS_PAGE_FAULT, fault_cause, "instruction page fault");
#endif

    // the syscall stub in the data page, which then gets EXECUTE instead of WRITE
    const size_t len = uspace_stub_syscall_end - uspace_stub_syscall_start;
    memcpy(data, uspace_stub_syscall_start, len);
    ASSERT_EQ(NO_ERROR, arch_mmu_protect(aspace, env.data, 1, kR | kX), "protect executable");
    ASSERT_TRUE(user_env_run(env, &retcode), "run");
    EXPECT_EQ(0, retcode, "jump once executable");
    EXPECT_EQ(1, syscalls, "syscalls");
    EXPECT_EQ(kSyscallMagic, syscall_arg, "syscall argument");

    // the thread is gone, so no cpu has the aspace loaded any more
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(env.as), "free aspace");
    env.as = nullptr;

    END_TEST;
}

#if ARCH_RISCV
// Kill a thread spinning in user mode on cpu. It is killed while preempted (cpu
// is this one) or while running (another cpu, interrupted by the kill); either
// way it meets the signal on its way back to user mode, and the arch's default
// kill hook ends it with ERR_INTERRUPTED.
bool kill_spinning_user_thread(const user_env &env, uint cpu) {
    BEGIN_TEST;

    uspace_thread_args args = { .env = &env };
    thread_t *t = thread_create("uspace_kill", uspace_thread, &args, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "thread_create");
    thread_set_pinned_cpu(t, cpu);
    thread_resume(t);
    // time to reach user mode; a kill sent before that is met at the first trap
    thread_sleep(20);
    thread_signal_kill(t);

    int retcode = 0;
    ASSERT_EQ(NO_ERROR, thread_join(t, &retcode, 5000), "the kill ends the thread");
    EXPECT_EQ(ERR_INTERRUPTED, retcode, "exit code set by the kill hook");

    END_TEST;
}

bool user_kill() {
    BEGIN_TEST;

    user_env env;
    ASSERT_TRUE(user_env_setup(&env), "setup");
    ASSERT_TRUE(user_env_load(&env, uspace_stub_spin_start, uspace_stub_spin_end), "load");

    const uint here = arch_curr_cpu_num();
    EXPECT_TRUE(kill_spinning_user_thread(env, here), "preempted on this cpu");
    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        if (cpu != here && mp_is_cpu_active(cpu)) {
            EXPECT_TRUE(kill_spinning_user_thread(env, cpu), "running on another cpu");
            break;
        }
    }

    // a cpu that ran the thread may still be switching away from the aspace
    status_t err;
    for (int i = 0; (err = vmm_free_aspace(env.as)) == ERR_BUSY && i < 100; i++) {
        thread_sleep(1);
    }
    EXPECT_EQ(NO_ERROR, err, "free aspace");
    env.as = nullptr;

    END_TEST;
}
#endif

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
RUN_TEST(user_protect);
#if ARCH_RISCV
RUN_TEST(user_kill);
#endif
END_TEST_CASE(arch_uspace_tests)

#endif // ARCH_ARM64 || (ARCH_RISCV && RISCV_S_MODE && RISCV_MMU)
