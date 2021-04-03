// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/auto_lock.h>
#include <fbl/ref_counted_upgradeable.h>
#include <fbl/ref_ptr.h>
#include <lib/zx/event.h>
#include <pthread.h>
#include <unittest/unittest.h>

namespace {
template <bool EnableAdoptionValidator>
class RawUpgradeTester :
    public fbl::RefCountedUpgradeable<RawUpgradeTester<EnableAdoptionValidator>,
                                      EnableAdoptionValidator> {
public:
    RawUpgradeTester(fbl::Mutex* mutex, fbl::atomic<bool>* destroying, zx::event* event)
        : mutex_(mutex), destroying_(destroying), destroying_event_(event) {}

    ~RawUpgradeTester() {
        atomic_store(destroying_, true);
        if (destroying_event_)
            destroying_event_->signal(0u, ZX_EVENT_SIGNALED);
        fbl::AutoLock al(mutex_);
    }

private:
    fbl::Mutex* mutex_;
    fbl::atomic<bool>* destroying_;
    zx::event* destroying_event_;
};

template <bool EnableAdoptionValidator>
void* adopt_and_reset(void* arg) {
    fbl::RefPtr<RawUpgradeTester<EnableAdoptionValidator>> rc_client =
        fbl::AdoptRef(reinterpret_cast<RawUpgradeTester<EnableAdoptionValidator>*>(arg));
    // The reset() which will call the dtor, which we expect to
    // block because upgrade_fail_test() is holding the mutex.
    rc_client.reset();
    return NULL;
}
} // namespace

template <bool EnableAdoptionValidator>
static bool upgrade_fail_test() {
    BEGIN_TEST;

    fbl::Mutex mutex;
    fbl::AllocChecker ac;
    fbl::atomic<bool> destroying{false};
    zx::event destroying_event;

    zx_status_t status = zx::event::create(0u, &destroying_event);
    ASSERT_EQ(status, ZX_OK);

    auto raw = new (&ac) RawUpgradeTester<EnableAdoptionValidator>(&mutex,
                                                                   &destroying,
                                                                   &destroying_event);
    EXPECT_TRUE(ac.check());

    pthread_t thread;
    {
        fbl::AutoLock al(&mutex);
        int res = pthread_create(&thread, NULL, &adopt_and_reset<EnableAdoptionValidator>, raw);
        ASSERT_LE(0, res);
        // Wait until the thread is in the destructor.
        status = destroying_event.wait_one(ZX_EVENT_SIGNALED, zx::time::infinite(), nullptr);
        EXPECT_EQ(status, ZX_OK);
        EXPECT_TRUE(atomic_load(&destroying));
        // The RawUpgradeTester must be blocked in the destructor, the upgrade will fail.
        auto upgrade1 = fbl::MakeRefPtrUpgradeFromRaw(raw, mutex);
        EXPECT_FALSE(upgrade1);
        // Verify that the previous upgrade attempt did not change the refcount.
        auto upgrade2 = fbl::MakeRefPtrUpgradeFromRaw(raw, mutex);
        EXPECT_FALSE(upgrade2);
    }

    pthread_join(thread, NULL);
    END_TEST;
}

template <bool EnableAdoptionValidator>
static bool upgrade_success_test() {
    BEGIN_TEST;

    fbl::Mutex mutex;
    fbl::AllocChecker ac;
    fbl::atomic<bool> destroying{false};

    auto ref = fbl::AdoptRef(new (&ac) RawUpgradeTester<EnableAdoptionValidator>(&mutex,
                                                                                 &destroying,
                                                                                 nullptr));
    EXPECT_TRUE(ac.check());
    auto raw = ref.get();

    {
        fbl::AutoLock al(&mutex);
        // RawUpgradeTester is not in the destructor so the upgrade should
        // succeed.
        auto upgrade = fbl::MakeRefPtrUpgradeFromRaw(raw, mutex);
        EXPECT_TRUE(upgrade);
    }

    ref.reset();
    EXPECT_TRUE(atomic_load(&destroying));

    END_TEST;
}

BEGIN_TEST_CASE(ref_counted_upgradeable_tests)
RUN_NAMED_TEST("Fail to upgrade raw pointer (adoption validation on)", upgrade_fail_test<true>)
RUN_NAMED_TEST("Fail to upgrade raw pointer (adoption validation off)", upgrade_fail_test<false>)
RUN_NAMED_TEST("Upgrade raw pointer (adoption validation on)", upgrade_success_test<true>)
RUN_NAMED_TEST("Upgrade raw pointer (adoption validation off)", upgrade_success_test<false>)
END_TEST_CASE(ref_counted_upgradeable_tests);
