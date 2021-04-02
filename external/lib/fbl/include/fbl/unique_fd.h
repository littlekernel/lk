// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdlib.h>
#include <unistd.h>

#include <fbl/macros.h>
#include <fbl/type_support.h>

namespace fbl {

// A scoped file descriptor that automatically closes when it goes
// out of scope.
class unique_fd {
public:
    constexpr unique_fd() : fd_(InvalidValue()) {}
    explicit unique_fd(int fd) : fd_(fd) { }

    static constexpr int InvalidValue() { return -1; }

    ~unique_fd() {
        reset();
    }

    unique_fd(unique_fd&& o) : fd_(o.release()) {}
    unique_fd& operator=(unique_fd&& o) {
        reset(o.release());
        return *this;
    }

    // Comparison against raw file descriptors (of the form fd == unique_fd)
    bool operator==(int fd) const { return (fd_ == fd); }
    bool operator!=(int fd) const { return (fd_ != fd); }

    // Comparison against other unique_fd's.
    bool operator==(const unique_fd& o) const { return fd_ == o.fd_; }
    bool operator!=(const unique_fd& o) const { return fd_ != o.fd_; }

    // move semantics only
    DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(unique_fd);

    int release() {
        int t = fd_;
        fd_ = InvalidValue();
        return t;
    }

    void reset(int t = InvalidValue()) {
        if (fd_ != InvalidValue()) {
            close(fd_);
        }
        fd_ = t;
    }

    void swap(unique_fd& other) {
        int t = fd_;
        fd_ = other.fd_;
        other.fd_ = t;
    }

    int get() const {
        return fd_;
    }

    bool is_valid() const  {
        return fd_ != InvalidValue();
    }

    explicit operator bool() const {
        return is_valid();
    }

    explicit operator int() const {
        return fd_;
    }

private:
    int fd_;
};

}  // namespace fbl
