/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <kernel/mutex.h>
#include <kernel/spinlock.h>
#include "nocopy.hpp"

// XXX quick lock wrappers. need to move somewhere else

namespace lk {

class automutex : nocopy {
public:
    automutex(mutex_t *mutex) : m_lock(mutex) {
        mutex_acquire(m_lock);
    }

    ~automutex() {
        release();
    }

    void release() {
        if (m_lock) {
            mutex_release(m_lock);
            m_lock = nullptr;
        }
    }

private:
    mutex_t *m_lock;
};

// XXX this doesn't seem to be terribly efficient on the release case
class autospinlock : nocopy {
public:
    autospinlock(spin_lock_t *lock, spin_lock_save_flags_t flags = SPIN_LOCK_FLAG_INTERRUPTS)
        : m_lock(lock), m_flags(flags) {
        spin_lock_save(m_lock, &m_state, m_flags);
    }

    ~autospinlock() {
        release();
    }

    void release() {
        if (m_lock) {
            spin_unlock_restore(m_lock, m_state, m_flags);
            m_lock = nullptr;
        }
    }

private:
    spin_lock_t *m_lock;
    spin_lock_save_flags_t m_flags;
    spin_lock_saved_state_t m_state;
};

};

