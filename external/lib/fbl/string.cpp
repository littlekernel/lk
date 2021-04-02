// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/string.h>

#include <string.h>

#include <zircon/assert.h>

#include <fbl/algorithm.h>
#include <fbl/atomic.h>
#include <fbl/new.h>

namespace fbl {
namespace {

size_t SumLengths(const String* begin, const String* end,
                  const String** last_non_empty_string) {
    size_t total_length = 0u;
    for (const String* it = begin; it != end; it++) {
        if (!it->empty()) {
            *last_non_empty_string = it;
            total_length += it->length();
        }
    }
    return total_length;
}

void Concat(char* data, const String* begin, const String* end) {
    for (const String* it = begin; it != end; it++) {
        memcpy(data, it->data(), it->length());
        data += it->length();
    }
    *data = 0;
}

} // namespace

String::EmptyBuffer String::gEmpty;

void String::clear() {
    ReleaseRef(data_);
    InitWithEmpty();
}

int String::compare(const String& other) const {
    size_t len = min(length(), other.length());
    int retval = memcmp(data(), other.data(), len);
    if (retval == 0) {
        if (length() == other.length()) {
            return 0;
        }
        return length() < other.length() ? -1 : 1;
    }
    return retval;
}

void String::swap(String& other) {
    char* temp_data = data_;
    data_ = other.data_;
    other.data_ = temp_data;
}

String& String::operator=(const String& other) {
    AcquireRef(other.data_);
    ReleaseRef(data_); // release after acquire in case other == *this
    data_ = other.data_;
    return *this;
}

String& String::operator=(String&& other) {
    ReleaseRef(data_);
    data_ = other.data_;
    other.InitWithEmpty();
    return *this;
}

void String::Set(const char* data, size_t length) {
    char* temp_data = data_;
    Init(data, length);
    ReleaseRef(temp_data); // release after init in case data is within data_
}

void String::Set(const char* data, size_t length, fbl::AllocChecker* ac) {
    char* temp_data = data_;
    Init(data, length, ac);
    ReleaseRef(temp_data); // release after init in case data is within data_
}

String String::Concat(initializer_list<String> strings) {
    const String* last_non_empty_string = nullptr;
    size_t total_length = SumLengths(strings.begin(), strings.end(),
                                     &last_non_empty_string);
    if (last_non_empty_string == nullptr) {
        return String();
    }
    if (total_length == last_non_empty_string->length()) {
        return *last_non_empty_string;
    }

    char* data = AllocData(total_length);

    fbl::Concat(data, strings.begin(), last_non_empty_string + 1);
    return String(data, nullptr);
}

String String::Concat(initializer_list<String> strings, AllocChecker* ac) {
    const String* last_non_empty_string = nullptr;
    size_t total_length = SumLengths(strings.begin(), strings.end(),
                                     &last_non_empty_string);
    if (last_non_empty_string == nullptr) {
        ac->arm(0u, true);
        return String();
    }
    if (total_length == last_non_empty_string->length()) {
        ac->arm(0u, true);
        return *last_non_empty_string;
    }

    char* data = AllocData(total_length, ac);
    if (!data) {
        return String();
    }

    fbl::Concat(data, strings.begin(), last_non_empty_string + 1);
    return String(data, nullptr);
}

void String::Init(const char* data, size_t length) {
    if (length == 0u) {
        InitWithEmpty();
        return;
    }

    data_ = AllocData(length);
    memcpy(data_, data, length);
    data_[length] = 0u;
}

void String::Init(const char* data, size_t length, AllocChecker* ac) {
    if (length == 0u) {
        ac->arm(0u, true);
        InitWithEmpty();
        return;
    }

    data_ = AllocData(length, ac);
    if (!data_) {
        InitWithEmpty();
        return;
    }
    memcpy(data_, data, length);
    data_[length] = 0u;
}

void String::Init(size_t count, char ch) {
    if (count == 0u) {
        InitWithEmpty();
        return;
    }

    data_ = AllocData(count);
    memset(data_, ch, count);
    data_[count] = 0u;
}

void String::Init(size_t count, char ch, AllocChecker* ac) {
    if (count == 0u) {
        ac->arm(0u, true);
        InitWithEmpty();
        return;
    }

    data_ = AllocData(count, ac);
    if (!data_) {
        InitWithEmpty();
        return;
    }
    memset(data_, ch, count);
    data_[count] = 0u;
}

void String::InitWithEmpty() {
    gEmpty.ref_count.fetch_add(1u, memory_order_relaxed);
    data_ = &gEmpty.nul;
}

char* String::AllocData(size_t length) {
    void* buffer = operator new(buffer_size(length));
    return InitData(buffer, length);
}

char* String::AllocData(size_t length, AllocChecker* ac) {
    void* buffer = operator new(buffer_size(length), ac);
    if (!buffer)
        return nullptr;
    return InitData(buffer, length);
}

char* String::InitData(void* buffer, size_t length) {
    char* data = static_cast<char*>(buffer) + kDataFieldOffset;
    *length_field_of(data) = length;
    new (ref_count_field_of(data)) atomic_uint(1u);
    return data;
}

void String::AcquireRef(char* data) {
    ref_count_field_of(data)->fetch_add(1u, memory_order_relaxed);
}

void String::ReleaseRef(char* data) {
    unsigned int prior_count = ref_count_field_of(data)->fetch_sub(1u, memory_order_release);
    ZX_DEBUG_ASSERT(prior_count != 0u);
    if (prior_count == 1u) {
        atomic_thread_fence(memory_order_acquire);
        operator delete(data - kDataFieldOffset);
    }
}

bool operator==(const String& lhs, const String& rhs) {
    return lhs.length() == rhs.length() &&
           memcmp(lhs.data(), rhs.data(), lhs.length()) == 0;
}

} // namespace fbl
