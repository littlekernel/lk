/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/bcache.h>
#include <lk/cpp.h>
#include <lk/err.h>

// C++ helper routine to hold a reference to a block in the block cache,
// mostly for RAII purposes.
class bcache_block_ref {
public:
    bcache_block_ref(bcache_t cache) : cache_(cache) {}
    ~bcache_block_ref() {
        close();
    }
    // move constructor
    bcache_block_ref(bcache_block_ref &&other) : bcache_block_ref(other.cache_) {
        // TODO: replace with equivalent to std::swap when/if implemented
        ptr_ = other.ptr_;
        block_num_ = other.block_num_;
        other.cache_ = {};
        other.ptr_ = {};
        other.block_num_ = {};
    }

    // move copy constructor
    bcache_block_ref &operator=(bcache_block_ref &&other) {
        cache_ = other.cache_;
        ptr_ = other.ptr_;
        block_num_ = other.block_num_;
        other.cache_ = {};
        other.ptr_ = {};
        other.block_num_ = {};
        return *this;
    }

    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(bcache_block_ref);

    // close out the current block
    int close() {
        int err = 0;
        if (ptr_) {
            err = bcache_put_block(cache_, block_num_);
            ptr_ = nullptr;
        }
        return err;
    }

    // get a new block
    int get_block(uint block) {
        // if it's already open just return it
        if (ptr_ && block_num_ == block) {
            return NO_ERROR;
        }

        // close out the existing block
        int err = close();
        if (err != 0) {
            return err;
        }

        // open the new one
        void *newptr;
        err = bcache_get_block(cache_, &newptr, block);
        if (err < 0) {
            return err;
        }

        ptr_ = newptr;
        block_num_ = block;

        return err;
    }

    void mark_dirty() {
        if (ptr_) {
            bcache_mark_block_dirty(cache_, block_num_);
        }
    }

    bool is_valid() const { return ptr_; }

    // return the pointer to the block
    const void *ptr() const { return ptr_; }
    void *ptr() { return ptr_; }

private:
    bcache_t cache_ {};
    void *ptr_ {};
    uint block_num_;
};
