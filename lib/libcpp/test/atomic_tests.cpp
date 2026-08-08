//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// Tests for lib/libcpp's <atomic>. These are single-threaded semantic
// checks of the API surface; the SMP correctness of the underlying
// __atomic builtins is the compiler's problem.

#include <atomic>
#include <kernel/thread.h>
#include <lib/unittest.h>

// std::atomic rejects instantiation on targets without always-lock-free
// atomics (e.g. 386-class x86), so there is nothing to test there.
#if __GCC_ATOMIC_INT_LOCK_FREE == 2 && __GCC_ATOMIC_POINTER_LOCK_FREE == 2 && \
    __GCC_ATOMIC_BOOL_LOCK_FREE == 2

namespace {

enum class Color : int { red = 1, green = 2 };

static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<void *>::is_always_lock_free);
static_assert(sizeof(std::atomic<int>) == sizeof(int));
static_assert(sizeof(std::atomic<void *>) == sizeof(void *));

bool atomic_int_ops() {
    BEGIN_TEST;

    std::atomic<int> a(5);
    EXPECT_EQ(5, a.load(), "");
    EXPECT_TRUE(a.is_lock_free(), "");

    a.store(7);
    EXPECT_EQ(7, a.load(std::memory_order_acquire), "");

    EXPECT_EQ(7, a.exchange(9), "");
    EXPECT_EQ(9, a.load(), "");

    EXPECT_EQ(9, a.fetch_add(3), "");
    EXPECT_EQ(12, a.load(), "");
    EXPECT_EQ(12, a.fetch_sub(2, std::memory_order_acq_rel), "");
    EXPECT_EQ(10, a.load(), "");

    std::atomic<unsigned> bits(0xf0f0u);
    EXPECT_EQ(0xf0f0u, bits.fetch_and(0xff00u), "");
    EXPECT_EQ(0xf000u, bits.load(), "");
    bits.fetch_or(0x000fu);
    EXPECT_EQ(0xf00fu, bits.load(), "");
    bits.fetch_xor(0xffffu);
    EXPECT_EQ(0x0ff0u, bits.load(), "");

    // operator sugar
    a = 1;
    EXPECT_EQ(2, ++a, "");
    EXPECT_EQ(2, a++, "");
    EXPECT_EQ(3, a.load(), "");
    EXPECT_EQ(2, --a, "");
    EXPECT_EQ(2, a--, "");
    EXPECT_EQ(1, a.load(), "");
    EXPECT_EQ(11, a += 10, "");
    EXPECT_EQ(6, a -= 5, "");
    int plain = a;
    EXPECT_EQ(6, plain, "implicit conversion loads");

    END_TEST;
}

bool atomic_compare_exchange() {
    BEGIN_TEST;

    std::atomic<int> a(10);

    int expected = 10;
    EXPECT_TRUE(a.compare_exchange_strong(expected, 20), "");
    EXPECT_EQ(20, a.load(), "");

    // failing exchange writes the observed value into expected
    expected = 999;
    EXPECT_FALSE(a.compare_exchange_strong(expected, 30), "");
    EXPECT_EQ(20, expected, "");
    EXPECT_EQ(20, a.load(), "");

    // weak may fail spuriously; loop it the way real code does
    expected = 20;
    while (!a.compare_exchange_weak(expected, 40)) {
    }
    EXPECT_EQ(40, a.load(), "");

    // explicit success/failure orderings
    expected = 40;
    EXPECT_TRUE(a.compare_exchange_strong(expected, 50, std::memory_order_acq_rel,
                                          std::memory_order_acquire),
                "");
    EXPECT_EQ(50, a.load(), "");

    END_TEST;
}

bool atomic_pointer_ops() {
    BEGIN_TEST;

    static int arr[8];
    std::atomic<int *> p(&arr[2]);

    EXPECT_TRUE(p.load() == &arr[2], "");

    // pointer arithmetic steps by elements, not bytes
    EXPECT_TRUE(p.fetch_add(3) == &arr[2], "");
    EXPECT_TRUE(p.load() == &arr[5], "");
    EXPECT_TRUE(p.fetch_sub(1) == &arr[5], "");
    EXPECT_TRUE(p.load() == &arr[4], "");
    EXPECT_TRUE(++p == &arr[5], "");
    EXPECT_TRUE(--p == &arr[4], "");
    EXPECT_TRUE((p += 2) == &arr[6], "");
    EXPECT_TRUE((p -= 4) == &arr[2], "");

    int *expected = &arr[2];
    EXPECT_TRUE(p.compare_exchange_strong(expected, &arr[7]), "");
    EXPECT_TRUE(p.load() == &arr[7], "");

    END_TEST;
}

bool atomic_enum_and_bool() {
    BEGIN_TEST;

    std::atomic<Color> c(Color::red);
    EXPECT_TRUE(c.load() == Color::red, "");
    c.store(Color::green);
    EXPECT_TRUE(c.exchange(Color::red) == Color::green, "");

    Color expected = Color::red;
    EXPECT_TRUE(c.compare_exchange_strong(expected, Color::green), "");

    std::atomic<bool> b(false);
    EXPECT_FALSE(b.load(), "");
    EXPECT_FALSE(b.exchange(true), "");
    EXPECT_TRUE(b.load(), "");

    END_TEST;
}

// 64-bit atomics, only on targets where std::atomic accepts them. The gate
// must be the same __atomic_always_lock_free() the header asserts on (the
// __GCC_ATOMIC_LLONG_LOCK_FREE macro over-reports: gcc calls 8-byte atomics
// "lock-free" on i686 because libatomic's implementation is, but will not
// inline them). That is not a preprocessor constant, so the test body is a
// template whose discarded branch never instantiates atomic<uint64_t>.
constexpr bool k64BitAtomics = __atomic_always_lock_free(sizeof(uint64_t), nullptr);

template <bool Enabled>
bool atomic_u64_ops_impl() {
    BEGIN_TEST;

    if constexpr (Enabled) {
        // dependent on Enabled so the false instantiation skips it entirely
        using U = std::conditional_t<Enabled, uint64_t, uint32_t>;

        std::atomic<U> a(0x100000000ULL);
        EXPECT_TRUE(a.load() == 0x100000000ULL, "");
        EXPECT_TRUE(a.fetch_add(1) == 0x100000000ULL, "");
        EXPECT_TRUE(a.load() == 0x100000001ULL, "");

        U expected = 0x100000001ULL;
        EXPECT_TRUE(a.compare_exchange_strong(expected, 0x200000000ULL), "");
        EXPECT_TRUE(a.exchange(7) == 0x200000000ULL, "");

        std::atomic<U> b(1);
        b.fetch_or(0x8000000000000000ULL);
        EXPECT_TRUE(b.load() == 0x8000000000000001ULL, "");
        static_assert(std::is_same_v<std::atomic_uint64_t, std::atomic<U>>,
                      "typedef instantiation");
    } else {
        unittest_printf(" (skipped: no lock-free 64-bit atomics)");
    }

    END_TEST;
}

bool atomic_u64_ops() {
    return atomic_u64_ops_impl<k64BitAtomics>();
}

// instantiate a representative sample of the typedefs
static_assert(sizeof(std::atomic_size_t) == sizeof(size_t));
static_assert(sizeof(std::atomic_uintptr_t) == sizeof(uintptr_t));
static_assert(sizeof(std::atomic_uint32_t) == sizeof(uint32_t));
static_assert(std::atomic_char(0).is_always_lock_free);
static_assert(sizeof(std::atomic_short) == sizeof(short));

bool atomic_flag_and_fences() {
    BEGIN_TEST;

    std::atomic_flag f;
    EXPECT_FALSE(f.test_and_set(), "first test_and_set observes clear");
    EXPECT_TRUE(f.test_and_set(), "second observes set");
    f.clear();
    EXPECT_FALSE(f.test_and_set(std::memory_order_acquire), "");

    // fences are exercised for compilation/linkage; nothing to observe
    // single-threaded
    std::atomic_thread_fence(std::memory_order_seq_cst);
    std::atomic_signal_fence(std::memory_order_release);
    EXPECT_EQ(1, std::kill_dependency(1), "");

    END_TEST;
}

// a light cross-thread smoke test: two threads hammer fetch_add and the
// total must come out exact
constexpr int kIncsPerThread = 100000;

int incrementer(void *arg) {
    auto *counter = static_cast<std::atomic<int> *>(arg);
    for (int i = 0; i < kIncsPerThread; i++) {
        counter->fetch_add(1, std::memory_order_relaxed);
    }
    return 0;
}

bool atomic_cross_thread() {
    BEGIN_TEST;

    std::atomic<int> counter(0);

    thread_t *t1 = thread_create("atomic_test_1", &incrementer, &counter,
                                 DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_t *t2 = thread_create("atomic_test_2", &incrementer, &counter,
                                 DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_TRUE(t1 != nullptr, "");
    ASSERT_TRUE(t2 != nullptr, "");
    thread_resume(t1);
    thread_resume(t2);
    thread_join(t1, nullptr, INFINITE_TIME);
    thread_join(t2, nullptr, INFINITE_TIME);

    EXPECT_EQ(2 * kIncsPerThread, counter.load(), "no increments may be lost");

    END_TEST;
}

BEGIN_TEST_CASE(libcpp_atomic_tests)
RUN_TEST(atomic_int_ops)
RUN_TEST(atomic_compare_exchange)
RUN_TEST(atomic_pointer_ops)
RUN_TEST(atomic_enum_and_bool)
RUN_TEST(atomic_u64_ops)
RUN_TEST(atomic_flag_and_fences)
RUN_TEST(atomic_cross_thread)
END_TEST_CASE(libcpp_atomic_tests)

} // namespace

#endif // always-lock-free atomics available
